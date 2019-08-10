#include "gb/emulator.hpp"
#include "emu/helper.hpp"
#undef min

namespace gb {

	//Split rom buffer into separate banks
	_inline_ List<emu::MemoryBanks> makeBanks(const Buffer &rom) {

		if (rom.size() < 0x14D)
			oic::System::log()->fatal("ROM requires a minimum size of 0x14D");

		usz x = 0;

		for (usz i = 0x134; i < 0x14D; ++i)
			x -= usz(u8(rom[i]) + 1);

		if(u8(x) != rom[0x14D])
			oic::System::log()->fatal("ROM has an invalid checksum");

		usz romBanks = rom[0x148];

		switch (romBanks) {
			case 52: romBanks = 72;						break;
			case 53: romBanks = 80;						break;
			case 54: romBanks = 96;						break;
			default: romBanks = 2 << romBanks;
		}

		usz ramBankSize = 8, ramBanks = rom[0x149];

		switch (ramBanks) {
			case 0: ramBankSize = 0;					break;
			case 1: ramBankSize = 2;					break;
			case 2: ramBanks = 1;						break;
			case 3: ramBanks = 4;						break;
			case 4: ramBanks = 16;						break;
			case 5: ramBanks = 8;						break;
			default:
				oic::System::log()->fatal("RAM banks are invalid");
		}

		return List<emu::MemoryBanks>{ 
			emu::MemoryBanks(romBanks, 16_KiB, rom, 16_KiB, rom.size()),
			emu::MemoryBanks(ramBanks, ramBankSize)
		};
	}

	Emulator::Emulator(const Buffer &rom): memory(
		List<Memory::Range>{
			Memory::Range { 0x0000_u16, u16(16_KiB), false, "ROM #0", "ROM bank #0", Buffer(rom.data(), rom.data() + 16_KiB) },
			Memory::Range { 0x8000_u16, u16(8_KiB), true, "VRAM", "Video memory", {} },
			Memory::Range{ 0xA000_u16, u16(12_KiB), true, "RAM #0", "RAM bank #0", {} },
			Memory::Range{ 0xFE00_u16, 512_u16, true, "I/O", "Input Output registers", {} }
		},
		makeBanks(rom) 
	) { }

	_inline_ usz step(Registers &r, Emulator::Memory &mem);

	void Emulator::wait() {

		usz cycles{};

		while (true)
			cycles += step(r, memory);
	}

	//For our switch case of 256 entries
	#define case1(x) case x: return op<x>(r, mem);
	#define case2(x) case1(x) case1(x + 1)
	#define case4(x) case2(x) case2(x + 2)
	#define case8(x) case4(x) case4(x + 4)
	#define case16(x) case8(x) case8(x + 8)
	#define case32(x) case16(x) case16(x + 16)
	#define case64(x) case32(x) case32(x + 32)
	#define case128(x) case64(x) case64(x + 64)
	#define case256() case128(0) case128(128)

	//LD r1, r2
	//LD (HL), r
	//LD r, (HL)
	//HALT

	static _inline_ usz halt() {
		//TODO: HALT
		return 16;
	}

	static _inline_ usz stop() {
		//TODO: Stop
		return 4;
	}

	template<u8 c> static _inline_ usz ld(Registers &r, Emulator::Memory &mem) {

		mem; r;

		constexpr u8 r1 = (c >> 3) & 7, r2 = c & 7;

		//r2 = (HL) and HALT
		if constexpr (r2 == 6) {

			//HALT
			if constexpr (r1 == 6) return halt();
			
			//r2 = (HL)
			else {
				r.regs[r1] = mem.get<u8>(r.hl);
				return 8;
			}
		}

		//r1 = (HL)
		else if constexpr (r1 == 6) {
			mem.set<u8>(r.hl, r.regs[r2]);
			return 8;
		}

		//Normal LD
		else {
			r.regs[r1] = r.regs[r2];
			return 4;
		}
	}
	//All alu operations (ADD/ADC/SUB/SUBC/AND/XOR/OR/CP)

	template<u8 c> static _inline_ usz performAlu(PSR &f, u8 &a, u8 b) {

		constexpr u8 code = c & 0x3F;

		if		constexpr (code < 0x08) emu::addTo(f, a, b);
		else if constexpr (code < 0x10) emu::adcTo(f, a, b);
		else if constexpr (code < 0x18) emu::subFrom(f, a, b);
		else if constexpr (code < 0x20) emu::sbcFrom(f, a, b);
		else if constexpr (code < 0x28) emu::andInto(f, a, b);
		else if constexpr (code < 0x30) emu::eorInto(f, a, b);
		else if constexpr (code < 0x38) emu::orrInto(f, a, b);
		else							emu::sub(f, a, b);

		return 4;
	}

	template<u8 c> static _inline_ usz aluOp(Registers &r, Emulator::Memory &mem) {

		//(HL) or (pc)
		if constexpr ((c & 7) == 6) {

			if constexpr (c < 0xC0)
				return performAlu<c>(r.f, r.a, mem.get<u8>(r.hl));
			else {
				usz v = performAlu<c>(r.f, r.a, mem.get<u8>(r.pc));
				++r.pc;
				return v;
			}
		}
		
		//Reg ALU
		else return performAlu<c>(r.f, r.a, r.regs[c & 7]);
	}

	//RST x instruction

	template<u8 c> static _inline_ usz rst(Registers &r, Emulator::Memory &m) {
		Emulator::Stack::push(m, r.sp, r.pc);
		r.pc = u16(c - 0xC0) | (c & 0x8);
		return 16;
	}

	//Condtional calls
	template<u8 c> static _inline bool cond(PSR &f) {

		constexpr u8 condition = (c / 8) % 4;

		if		constexpr (condition == 0) return !f.zero();
		else if constexpr (condition == 1) return f.zero();
		else if constexpr (condition == 2) return !f.carry();
		else							   return f.carry();
	}

	//JR/JP calls

	template<bool jp, u8 check> static _inline_ usz branch(Registers &r, Emulator::Memory &m) {
		
		if constexpr (check != 0)
			if (!cond<check>(r.f))
				if constexpr (jp)
					return 12;
				else
					return 8;

		if constexpr (jp) {
			r.pc = m.get<u16>(r.pc);
			return 16;
		} else {
			r.pc += u8(m.get<u8>(r.pc));
			++r.pc;
			return 12;
		}
	}

	template<u8 c> static _inline_ usz jmp(Registers &r, Emulator::Memory &m) {

		r; m;

		if		constexpr (c == 0x10) return stop();
		else if constexpr (c == 0x18) return branch<false, 0>(r, m);
		else if constexpr (c == 0xC3) return branch<true, 0>(r, m);
		else if constexpr (c < 0x40) return branch<false, c>(r, m);
		else return branch<true, c>(r, m);
	}

	//Every case runs through this

	template<u8 c> static _inline_ usz op(Registers &r, Emulator::Memory &mem) {

		mem; r;

		if constexpr (c == 0x00)			//NOP
			return 4;

		else if constexpr (c < 0x40) {

			if constexpr (c == 0x18 || c == 0x10)return jmp<c>(r, mem);

			else if constexpr (c < 0x20)		//LD shorts
				throw std::exception();

			else if constexpr ((c & 0x07) == 0)	return jmp<c>(r, mem);
			else
				throw std::exception();
		}

		else if constexpr (c < 0x80)			return ld<c>(r, mem);
		else if constexpr (c < 0xC0)			return aluOp<c>(r, mem);
		else if constexpr ((c & 0x7) == 0x6)	return aluOp<c>(r, mem);
		else if constexpr ((c & 0x7) == 0x7)	return rst<c>(r, mem);

		else if constexpr (c == 0xC3 || ((c & 0x7) == 2 && c < 0xE0))
			return jmp<c>(r, mem);

		else										//Undefined operation
			throw std::exception();
	}

	//Switch case of all opcodes

	_inline_ usz step(Registers &r, Emulator::Memory &mem) {

		u8 opCode = mem.get<u8>(r.pc);
		++r.pc;

		switch (opCode) {
			case256()
		}

		return 0;
	}

}