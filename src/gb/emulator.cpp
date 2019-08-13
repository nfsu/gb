#include "gb/emulator.hpp"
#include "emu/helper.hpp"
#undef min

namespace gb {

	//Memory emulation

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
			case 0: ramBanks = 1;						break;
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
			emu::MemoryBanks(ramBanks, ramBankSize * 1_KiB)
		};
	}

	Emulator::Emulator(const Buffer &rom): memory(
		List<Memory::Range>{
			Memory::Range{ 0x0000_u16, u16(16_KiB), false, "ROM #0", "ROM bank #0", Buffer(rom.data(), rom.data() + 16_KiB) },
			Memory::Range{ 0x8000_u16, u16(8_KiB), true, "VRAM", "Video memory", {} },
			Memory::Range{ 0xA000_u16, u16(8_KiB), true, "SAV", "Cartridge RAM", {} },
			Memory::Range{ 0xC000_u16, u16(4_KiB), true, "RAM #0", "RAM bank #0", {} },
			Memory::Range{ 0xFE00_u16, 512_u16, true, "I/O", "Input Output registers", {} }
		},
		makeBanks(rom) 
	) { }

	template<typename T, typename Memory, usz mapping>
	_inline_ usz IOHandler::exec(Memory *m, u16 addr, const T *t) {
			
		if (addr < 0x8000)
			return mapping | 0xFEA0;	//Write into unused memory

		t; m;

		switch (addr & 0xFF) {

			case 0xF:

				if (m->values[0] & Emulator::IME) {

					const u8 u = u8(*t);
					const u8 f = *(const u8*)(mapping | 0xFFFF), g = u & f;

					if(g & 1)		//V-Blank
						oic::System::log()->println("V-Blank");

					if(g & 2)		//LCD STAT
						oic::System::log()->println("LCD STAT");

					if(g & 4)		//Timer
						oic::System::log()->println("Timer");

					if(g & 8)		//Serial
						oic::System::log()->println("Serial");

					if(g & 16)		//Joypad
						oic::System::log()->println("Joypad");

					m->set(addr, u8(u & ~g));

				}

				break;

		}
		
		return addr;
	}

	//CPU/GPU emulation

	_inline_ usz cpuStep(Registers &r, Emulator::Memory &mem);
	_inline_ usz ppuStep(usz ppuCycle, Emulator::Memory &mem);

	void Emulator::wait() {

		usz cycles{}, ppuCycle{};

		while (true) {
			cycles += cpuStep(r, memory);
			ppuCycle = ppuStep(ppuCycle, memory);
		}
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

	//Function for setting a cr

	template<u8 cr, typename T> static _inline_ void set(Registers &r, Emulator::Memory &mem, const T &t) {
		mem;
		if constexpr (cr != 6) r.regs[cr] = t;
		else mem.set(r.hl, t);
	}

	template<u8 c, typename T> static _inline_ void setc(Registers &r, Emulator::Memory &mem, const T &t) { 
		set<(c >> 3) & 7, T>(r, mem, t);
	}

	//Function for getting a cr

	template<u8 cr, typename T> static _inline_ T get(Registers &r, Emulator::Memory &mem) {
		mem;
		if constexpr(cr == 6) return mem.get<T>(r.hl);
		else if constexpr ((cr & 0x8) != 0) {
			const T t = mem.get<T>(r.pc);
			r.pc += sizeof(T);
			return t;
		} else return r.regs[cr];
	}

	template<u8 c, typename T> static _inline_ T getc(Registers &r, Emulator::Memory &mem) {
		return get<c & 7, T>(r, mem);
	}

	//Special instructions

	static _inline_ usz halt() {
		//TODO: HALT
		return 16;
	}

	static _inline_ usz stop() {
		//TODO: Stop
		return 4;
	}
	
	template<bool enable, usz loc = 0, usz flag = 1>
	static _inline_ void interrupt(Emulator::Memory &mem) {
		if constexpr (enable)
			mem.values[loc] |= flag;
		else
			mem.values[loc] &= ~flag;
	}

	//LD operations
		//LD cr, cr

	template<u8 c> static _inline_ usz ld(Registers &r, Emulator::Memory &mem) {
		setc<c, u8>(r, mem, getc<c, u8>(r, mem));
		return (c & 0x7) == 6 || ((c >> 3) & 7) == 6 ? 8 : 4;
	}

		//LD cr, (pc)

	template<u8 s> static _inline_ u16 addrFromReg(Registers &r) {
		if constexpr (s < 2)
			return r.lregs[s];
		else if constexpr (s == 2) {
			++r.hl;
			return r.hl - 1;
		} else {
			--r.hl;
			return r.hl + 1;
		}
	}

	template<u8 c> static _inline_ usz ldi(Registers &r, Emulator::Memory &mem) {
		if constexpr ((c & 7) == 6)
			setc<c, u8>(r, mem, get<8, u8>(r, mem));
		else if constexpr ((c & 0xF) == 2)
			mem.set<u8>(addrFromReg<u8(c >> 4)>(r), r.a);
		else
			r.a = mem.get<u8>(addrFromReg<u8(c >> 4)>(r));

		return c == 0x36 ? 12 : 8;
	}

		//LD A, (a16) / LD (a16), A

	template<u8 c> static _inline_ usz lds(Registers &r, Emulator::Memory &mem) {
		shortReg<c>(r) = mem.get<u16>(r.pc);
		r.pc += 2;
		return 12;
	}

	//All alu operations (ADD/ADC/SUB/SUBC/AND/XOR/OR/CP)

	template<u8 c> static _inline_ usz performAlu(PSR &f, u8 &a, u8 b) {

		static constexpr u8 code = c & 0x3F;

		if constexpr (code < 0x08)
			emu::addTo(f, a, b);

		else if constexpr (code < 0x10)
			emu::adcTo(f, a, b);

		else if constexpr (code < 0x18)
			emu::subFrom(f, a, b);

		else if constexpr (code < 0x20)
			emu::sbcFrom(f, a, b);

		else if constexpr (code < 0x28)
			emu::andInto(f, a, b);

		else if constexpr (code < 0x30)
			emu::eorInto(f, a, b);

		else if constexpr (code < 0x38)
			emu::orrInto(f, a, b);

		else
			emu::sub(f, a, b);

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
		else
			return performAlu<c>(r.f, r.a, r.regs[c & 7]);
	}

	//RST x instruction

	template<u8 c> static _inline_ usz rst(Registers &r, Emulator::Memory &m) {
		Emulator::Stack::push(m, r.sp, r.pc);
		r.pc = u16(c - 0xC0) | (c & 0x8);
		return 16;
	}

	//Condtional calls
	template<u8 c> static _inline_ bool cond(PSR &f) {

		static constexpr u8 condition = (c / 8) % 4;

		if constexpr (condition == 0)
			return !f.zero();

		else if constexpr (condition == 1)
			return f.zero();

		else if constexpr (condition == 2)
			return !f.carry();

		else
			return f.carry();
	}

	//JR/JP calls

	template<u8 jp, u8 check> static _inline_ usz branch(Registers &r, Emulator::Memory &m) {
		
		if constexpr (check != 0)
			if (!cond<check>(r.f))
				if constexpr (inRegion<jp, 1, 3>) {			//2-width instructions
					r.pc += 2;
					return 12;
				} else if constexpr((jp & 3) == 0) {		//1-width instructions
					++r.pc;
					return 8;
				} else
					return 8;

		if constexpr ((jp & 3) == 3) {						//RET

			Emulator::Stack::pop(m, r.sp, r.pc);

			if constexpr ((jp & 8) != 0)
				interrupt<true>(m);

			return check == 0 ? 16 : 20;

		} else if constexpr (inRegion<jp & 3, 1, 3>) {		//JP/CALL

			if constexpr ((jp & 3) == 2)					//CALL
				Emulator::Stack::push(m, r.sp, r.pc);

			if constexpr ((jp & 4) != 0)
				r.pc = m.get<u16>(r.hl);
			else
				r.pc = m.get<u16>(r.pc);

			return 8 + jp * 8;

		} else {											//JR
			r.pc += u16(i16(m.get<i8>(r.pc)));
			++r.pc;
			return 12;
		}
	}

	template<u8 c, u8 code> static _inline_ usz jmp(Registers &r, Emulator::Memory &m) {

		r; m;

		if constexpr (c == 0x18)
			return branch<0, 0>(r, m);

		else if constexpr (c < 0x40)
			return branch<0, c>(r, m);

		else if constexpr (c == 0xC3)
			return branch<1, 0>(r, m);

		else if constexpr (code == 2)
			return branch<1, c>(r, m);

		else if constexpr (c == 0xCD)
			return branch<2, 0>(r, m);

		else if constexpr (code == 4)
			return branch<2, c>(r, m);

		else if constexpr (c == 0xE9)
			return branch<4, 0>(r, m);

		else if constexpr (c == 0xC9)
			return branch<7, 0>(r, m);		//RET

		else if constexpr (c == 0xD9)
			return branch<15, 0>(r, m);		//RETI

		else
			return branch<3, c>(r, m);		//RET conditional
	}

	//INC/DEC x instructions

	template<u8 c> static _inline_ usz inc(Registers &r, Emulator::Memory &mem) {

		static constexpr u8 add = c & 1 ? u8_MAX : 1;

		if constexpr ((c & 1) == 0)
			r.f.clearSubtract();
		else
			r.f.setSubtract();

		u8 v;
		static constexpr u8 r1 = c / 8;

		if constexpr (r1 != 6)
			v = r.regs[r1] += add;

		else
			v = mem.set(r.hl, u8(mem.get<u8>(r.hl) + add));

		r.f.carryHalf(v & 0x10);
		r.f.zero(v == 0);
		return r1 == 6 ? 12 : 4;
	}

	//Short register functions

	template<u8 c> static _inline_ u16 &shortReg(Registers &r) {
		if constexpr (c / 16 < 3)
			return r.lregs[c / 16];
		else
			return r.sp;
	}

	template<u8 c> static _inline_ usz incs(Registers &r) {
		static constexpr u16 add = c & 8 ? u16_MAX : 1;
		shortReg<c>(r) += add;
		return 8;
	}

	//Every case runs through this

	template<u8 c, u8 start, u8 end> static constexpr bool inRegion = c >= start && c < end;

	template<u8 c, u8 code, u8 hi> static constexpr bool isJump = 
		(inRegion<c, 0x18, 0x40> && code == 0) ||	//JR
		(inRegion<c, 0xC0, 0xE0> && (				//JP/RET/CALL
			code == 0 ||							//RET conditional
			code == 2 ||							//JP conditional
			code == 4 ||							//CALL conditional
			c == 0xC3 ||							//JP
			hi == 9	  ||							//RET/RETI
			c == 0xCD								//CALL
		)) || c == 0xE9;							//JP (HL)

	enum OpCode : u8 {
		NOP = 0x00, STOP = 0x10, HALT = 0x76,
		DI = 0xF3, EI = 0xFB
	};

	template<u8 c> static _inline_ usz op(Registers &r, Emulator::Memory &mem) {

		mem; r;

		static constexpr u8 code = c & 0x07, hi = c & 0xF;

		if constexpr (c == NOP)
			return 4;

		else if constexpr (c == STOP)
			return stop();

		else if constexpr (c == HALT)
			return halt();

		else if constexpr (c == DI || c == EI) {		//DI/EI; disable/enable interrupts
			usz cycles = 4 + cpuStep(r, mem);
			interrupt<c == EI>(mem);
			return cycles;
		}

		else if constexpr (isJump<c, code, hi>)	//RET, JP, JR and CALL
			return jmp<c, code>(r, mem);

		else if constexpr (c < 0x40) {

			if constexpr ((c & 0xF) == 1)
				return lds<c>(r, mem);

			else if constexpr (code == 3)
				return incs<c>(r);

			else if constexpr (code == 4 || code == 5)
				return inc<c>(r, mem);

			else if constexpr (code == 2 || code == 6)
				return ldi<c>(r, mem);

			else
				throw std::exception();
		}

		else if constexpr (c < 0x80)
			return ld<c>(r, mem);

		else if constexpr (c < 0xC0 || code == 0x6)
			return aluOp<c>(r, mem);

		else if constexpr (code == 0x7)
			return rst<c>(r, mem);

		//LD (a8/a16/C)/A, (a8/a16/C)/A
		else if constexpr (c >= 0xE0 && (hi == 0x0 || hi == 0x2 || hi == 0xA)) {

			u16 addr;

			if constexpr (hi == 0x0) {
				addr = 0xFF00 | mem.get<u8>(r.pc);
				++r.pc;
			} else if constexpr (hi == 0x2)
				addr = 0xFF00 | r.c;
			else {
				addr = mem.get<u16>(r.pc);
				r.pc += 2;
			}

			if constexpr (c < 0xF0)
				mem.set(addr, r.a);
			else
				r.a = mem.get<u8>(addr);

			return 4;
		}

		//TODO: RLCA/RRCA/RLA/RRA
		//TODO: ADD HL, lreg
		//TODO: PUSH/POP lreg
		//TODO: ADD SP, r8
		//TODO: LD HL SP, HL
		//TODO:	DAA/SCF/CPL/CCF

		else										//Undefined operation
			throw std::exception();
	}

	//Switch case of all opcodes

	_inline_ usz cpuStep(Registers &r, Emulator::Memory &mem) {

		u8 opCode = mem.get<u8>(r.pc);
		++r.pc;

		switch (opCode) {
			case256()
		}

		return 0;
	}

	_inline_ void pushLine(Emulator::Memory &) {
		//TODO:
	}

	_inline_ void pushScreen() {
		//TODO:
	}

	_inline_ void pushBlank() {
		//TODO:
	}

	_inline_ usz ppuStep(usz ppuCycle, Emulator::Memory &mem) {

		static constexpr u16 ctrl = 0xFF40, stat = 0xFF41, ly = 0xFF44;

		enum Modes {
			HBLANK = 0,			HBLANK_INTERVAL = 204,
			VBLANK = 1,			VBLANK_INTERVAL = 456,
			OAM = 2,			OAM_INTERVAL = 80,
			VRAM = 3,			VRAM_INTERVAL = 172
		};

		//Push populated frame
		if (mem.get<u8>(ctrl) & 0x80) {

			mem.values[Emulator::FLAGS] &= ~Emulator::LCD_CLEARED;

			u8 lcdc = mem.get<u8>(stat);
			u8 mode = lcdc & 3;

			switch (mode) {

				case HBLANK:

					if (ppuCycle == HBLANK_INTERVAL) {

						//Push to screen and restart vblank
						if (mem.increment<u8>(ly) == 143) {
							pushScreen();
							goto incrReset;
						}

						//Start scanline
						mode = OAM;
						goto reset;
					}

					break;

				case VBLANK:

					if (ppuCycle == VBLANK_INTERVAL) {

						if (mem.increment<u8>(ly) > 153) {
							mem.set<u8>(ly, 0);
							goto incrReset;
						}

						return 0;
					}
					
					break;

				case OAM:

					if (ppuCycle == OAM_INTERVAL)
						goto incrReset;
					
					break;

				case VRAM:

					if (ppuCycle == VRAM_INTERVAL) {
						pushLine(mem);
						goto incrReset;
					}

			}

			return ppuCycle + 1;

		incrReset:
			++mode;

		reset:

			mem.set<u8>(stat, (lcdc & ~3) | (mode & 3));
			return 0;
		}

		//Push blank frame
		else if (!(mem.values[Emulator::FLAGS] & Emulator::LCD_CLEARED)) {
			pushBlank();
			mem.values[Emulator::FLAGS] |= Emulator::LCD_CLEARED;
		}

		return ppuCycle;
	}

}