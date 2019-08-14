#include "gb/emulator.hpp"
#include "emu/helper.hpp"
#include "system/viewport_manager.hpp"
#undef min

namespace gb {

	//Memory emulation

	//Split rom buffer into separate banks
	_inline_ List<emu::ProgramMemoryRange> makeBanks(const Buffer &rom) {

		if (rom.size() < 0x14D)
			oic::System::log()->fatal("ROM requires a minimum size of 0x14D");

		usz x = 0;

		for (usz i = 0x134; i < 0x14D; ++i)
			x -= usz(u8(rom[i] + 1));

		if(u8(x) != rom[0x14D])
			oic::System::log()->fatal("ROM has an invalid checksum");

		usz romBankSize = 16_KiB, romBanks = rom[0x148];

		switch (romBanks) {
			case 52: romBanks = 72;						break;
			case 53: romBanks = 80;						break;
			case 54: romBanks = 96;						break;
			default: romBanks = usz(2 << romBanks);
		}

		usz ramBankSize = 8_KiB, ramBanks = rom[0x149];

		switch (ramBanks) {
			case 0: ramBanks = 1;						break;
			case 1: ramBankSize = 2_KiB;				break;
			case 2: ramBanks = 1;						break;
			case 3: ramBanks = 4;						break;
			case 4: ramBanks = 16;						break;
			case 5: ramBanks = 8;						break;
			default:
				oic::System::log()->fatal("RAM banks are invalid");
		}

		using Range = emu::ProgramMemoryRange;

		return List<Range>{
			Range{ MemoryMapper::cpuStart, MemoryMapper::cpuLength, true, "CPU", "CPU Memory", {}, false },
			Range{ MemoryMapper::ppuStart, MemoryMapper::ppuLength, true, "PPU", "Pixel Processing Unit", {} },
			Range{ MemoryMapper::ramStart, ramBankSize * ramBanks, true, "RAM #n", "RAM Banks", {} },
			Range{ MemoryMapper::romStart, romBankSize * romBanks, false, "ROM #n", "ROM Banks", rom },
			Range{ MemoryMapper::mmuStart, MemoryMapper::mmuLength, true, "MMU", "Memory Unit additional variables", {} }
		};
	}

	Emulator::Emulator(const Buffer &rom): memory(
		makeBanks(rom),
		List<Memory::Range>{
			Memory::Range{ 0x0000_u16, u16(32_KiB), false, "ROM", "Readonly memory", {}, false },
			Memory::Range{ 0x8000_u16, u16(8_KiB), true, "VRAM", "Video memory", {} },
			Memory::Range{ 0xA000_u16, u16(8_KiB), true, "SAV", "Cartridge RAM", {} },
			Memory::Range{ 0xC000_u16, u16(8_KiB), true, "RAM", "Random access memory", {}, false },
			Memory::Range{ 0xE000_u16, u16(0xFE00 - 0xE000), true, "ERAM", "Echo RAM", {}, false },
			Memory::Range{ 0xFE00_u16, 512_u16, true, "I/O", "Input Output registers", {} }
		}
	) { }

	template<typename T, typename Memory, usz mapping>
	_inline_ usz IOHandler::exec(Memory *m, u16 addr, const T *t) {
			
		if (addr < 0x8000)
			return mapping | 0xFEA0;	//Write into unused memory

		t; m;

		switch (addr & 0xFF) {

			case 0xF:

				if (m->getMemory<u8>(Emulator::FLAGS) & Emulator::IME) {

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

					m->getMemory<u8>(mapping | addr) = u8(u & ~g);

				}

				break;

		}
		
		return mapping | addr;
	}

	//CPU/GPU emulation

	_inline_ usz cpuStep(Registers &r, Emulator::Memory &mem);
	_inline_ usz ppuStep(usz ppuCycle, Emulator::Memory &mem);

	template<bool on>
	_inline_ void pushBlank();

	void Emulator::wait() {

		usz ppuCycle{};

		pushBlank<false>();

		while (true) {
			ppuCycle += cpuStep(r, memory);
			ppuCycle = ppuStep(ppuCycle, memory);
		}
	}

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
		return 1;
	}

	static _inline_ usz stop() {
		//TODO: Stop
		return 1;
	}
	
	template<bool enable, usz flag>
	static _inline_ void setFlag(Emulator::Memory &mem) {
		if constexpr (enable)
			mem.getMemory<u8>(Emulator::FLAGS) |= flag;
		else
			mem.getMemory<u8>(Emulator::FLAGS) &= ~flag;
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
		return 3;
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

		return 1;
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
		return 4;
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
					return 3;
				} else if constexpr((jp & 3) == 0) {		//1-width instructions
					++r.pc;
					return 2;
				} else
					return 2;

		if constexpr ((jp & 3) == 3) {						//RET

			Emulator::Stack::pop(m, r.sp, r.pc);

			if constexpr ((jp & 8) != 0)
				setFlag<true, Emulator::IME>(m);

			return check == 0 ? 4 : 5;

		} else if constexpr (inRegion<jp & 3, 1, 3>) {		//JP/CALL

			if constexpr ((jp & 3) == 2)					//CALL
				Emulator::Stack::push(m, r.sp, r.pc);

			if constexpr ((jp & 4) != 0)
				r.pc = m.get<u16>(r.hl);
			else
				r.pc = m.get<u16>(r.pc);

			return 2 + jp * 2;

		} else {											//JR
			r.pc += u16(i16(m.get<i8>(r.pc)));
			++r.pc;
			return 3;
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
		return r1 == 6 ? 3 : 1;
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
		return 2;
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

	static _inline_ usz cbInstruction(Registers &r, Emulator::Memory &mem);

	template<u8 c> static _inline_ usz opCb(Registers &r, Emulator::Memory &mem) {

		static constexpr u8 p = (c >> 3) & 7;
		static constexpr u8 cr = c & 7;

		//Barrel shifts
		if constexpr (c < 0x40) {
			
			r.f.clearSubtract();
			r.f.clearHalf();

			u8 i = get<cr, u8>(r, mem), j = i;	j;

			//RLC (rotate to the left)
			if constexpr (p == 0)
				r.a = i = u8(i << 1) | u8(i >> 7);

			//RRC (rotate to the right)
			else if constexpr (p == 1)
				r.a = i = u8(i >> 1) | u8(i << 7);

			//RL (<<1 shift in carry)
			else if constexpr (p == 2)
				r.a = i = (i << 1) | u8(r.f.carry());

			//RR (>>1 shift in carry)
			else if constexpr (p == 3)
				r.a = i = (i >> 1) | u8(0x80 * r.f.carry());

			//SLA (a = cr << 1)
			else if constexpr (p == 4)
				r.a = i <<= 1;

			//SRA (a = cr >> 1 (maintain sign))
			else if constexpr (p == 5)
				r.a = i = (i >> 1) | (i & 0x80);

			//Swap two nibbles
			else if constexpr (p == 6)
				r.a = i = u8(i << 4) | (i >> 4);

			//SRL
			else
				r.a = i >>= 1;

			//Set carry
			if constexpr ((p & 1) == 1)
				r.f.carry(j & 1);
			else if constexpr (p < 6)
				r.f.carry(j & 0x80);
			else
				r.f.clearCarry();

			//Set zero
			r.f.zero(i == 0);
		}

		//Bit masks
		else if constexpr (c < 0x80) {
			r.f.setHalf();
			r.f.clearSubtract();
			r.f.zero(get<cr, u8>(r, mem) & (1 << p));
		}

		//Reset bit
		else if constexpr (c < 0xC0)
			set<cr, u8>(r, mem, get<cr, u8>(r, mem) & ~(1 << p));

		//Set bit
		else
			set<cr, u8>(r, mem, get<cr, u8>(r, mem) | (1 << p));

		return cr == 6 ? 4 : 2;
	}

	template<u8 c> static _inline_ usz op256(Registers &r, Emulator::Memory &mem) {

		mem; r;

		static constexpr u8 code = c & 0x07, hi = c & 0xF;

		if constexpr (c == NOP)
			return 1;

		else if constexpr (c == STOP)
			return stop();

		else if constexpr (c == HALT)
			return halt();

		//DI/EI; disable/enable interrupts
		else if constexpr (c == DI || c == EI) {
			setFlag<c == EI, Emulator::IME>(mem);
			return 1;
		}

		//RET, JP, JR and CALL
		else if constexpr (isJump<c, code, hi>)
			return jmp<c, code>(r, mem);

		//RLCA, RLA, RRCA, RRA
		else if constexpr (c < 0x20 && code == 7)
			return opCb<c>(r, mem);

		else if constexpr (c < 0x40) {

			if constexpr (hi == 1)
				return lds<c>(r, mem);

			else if constexpr (hi == 0x9) {
				u16 &a = shortReg<c>(r), hl = r.hl;
				r.f.clearSubtract();
				r.hl += a;
				r.f.carryHalf(r.hl & 0x10);
				r.f.carry(r.hl < hl);
				return 2;
			}

			else if constexpr (code == 3)
				return incs<c>(r);

			else if constexpr (code == 4 || code == 5)
				return inc<c>(r, mem);

			else if constexpr (code == 2 || code == 6)
				return ldi<c>(r, mem);

			//SCF, CCF
			else if constexpr (c == 0x37 || c == 0x3F) {

				r.f.clearSubtract();
				r.f.clearHalf();

				if constexpr (c == 0x37)
					r.f.setCarry();
				else
					r.f.carry(!r.f.carry());

				return 1;
			}

			//CPL
			else if constexpr (c == 0x2F) {
				r.f.setSubtract();
				r.f.setHalf();
				r.a = ~r.a;
				return 1;
			}

			//DAA
			else if constexpr (c == 0x27) {

				u8 i = 0, j = r.a;
				r.f.clearCarry();

				if (r.f.carryHalf() || (!r.f.subtract() && (j & 0xF) > 0x9))
					i |= 0x6;

				if (r.f.carry() || (!r.f.subtract() && j > 0x99)) {
					i |= 0x60;
					r.f.setCarry();
				}

				r.a = j += u8(-i8(i));
				r.f.clearHalf();
				r.f.zero(j == 0);
				return 1;
			}

			//TODO: LD (a16), SP

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

			return 1;
		}

		//CB instructions (mask, reset, set, rlc/rrc/rl/rr/sla/sra/swap/srl)
		else if constexpr (c == 0xCB)
			return cbInstruction(r, mem);

		//PUSH/POP
		else if constexpr (hi == 1 || hi == 5) {

			static constexpr u8 reg = (c - 0xC0) >> 4;

			//Flip bytes for pushing, since AF register is FA for us
			if constexpr (hi != 1)
				r.fa = (r.f.v << 8) | r.a;

			//Push to or pop from stack
			if constexpr (hi == 1)
				Emulator::Stack::pop(mem, r.sp, r.lregs[reg]);
			else
				Emulator::Stack::push(mem, r.sp, r.lregs[reg]);

			//Flip bytes for af instruction, since AF register is FA for us
			if constexpr (reg == 3)
				r.fa = (r.f.v << 8) | r.a;	//Flip bytes, since AF register is FA for us

			return hi == 1 ? 3 : 4;
		}

		//TODO: ADD SP, r8
		//TODO: LD HL, SP+a8
		//TODO: LD SP, HL

		else										//Undefined operation
			throw std::exception();
	}

	//Switch case of all opcodes

	//For our switch case of 256 entries
	#define case1(x,y) case x: return op##y<x>(r, mem);
	#define case2(x,y) case1(x,y) case1(x + 1,y)
	#define case4(x,y) case2(x,y) case2(x + 2,y)
	#define case8(x,y) case4(x,y) case4(x + 4,y)
	#define case16(x,y) case8(x,y) case8(x + 8,y)
	#define case32(x,y) case16(x,y) case16(x + 16,y)
	#define case64(x,y) case32(x,y) case32(x + 32,y)
	#define case128(x,y) case64(x,y) case64(x + 64,y)
	#define case256(y) case128(0,y) case128(128,y)

	static _inline_ usz cbInstruction(Registers &r, Emulator::Memory &mem) {

		u8 opCode = mem.get<u8>(r.pc);
		++r.pc;

		switch (opCode) {
			case256(Cb)
		}

		return 0;
	}

	_inline_ usz cpuStep(Registers &r, Emulator::Memory &mem) {

		u8 opCode = mem.get<u8>(r.pc);
		++r.pc;

		switch (opCode) {
			case256(256)
		}

		return 0;
	}

	//Palette

	constexpr u32 rgb(u8 r8, u8 g8, u8 b8) {
		return r8 | (g8 << 8) | (b8 << 16);
	}

	static constexpr u32 palette[5] = {
		rgb(136, 148, 87),	//Color 0
		rgb(84, 118, 89),	//Color 1
		rgb(59, 88, 76),	//Color 2
		rgb(34, 58, 50),	//Color 3
		rgb(66, 81, 3)		//OFF color
	};

	//Copy the immediate buffer to the screen
	_inline_ void pushScreen() {
		auto *vpm = oic::System::viewportManager();
		const auto &wind = vpm->operator[](0);
		vpm->redraw(wind);

		Sleep(1000);
	}

	//Render a line
	_inline_ void pushLine(Emulator::Memory &mem) {

		usz
			ly = mem.get<u8>(io::ly),
			wy = mem.get<u8>(io::wy),
			wx = mem.get<u8>(io::wx),
			y = (ly + mem.get<u8>(io::scy)) & 0xFF,
			scx = mem.get<u8>(io::scx),
			offy = y << 5,
			ctrl = mem.get<u8>(io::ctrl);

		wy; wx;

		u32 *ppu = (u32*)MemoryMapper::ppuStart;
		ppu += ly * specs::width;

		u16 offset = 0x8000;

		if (ctrl & (io::enableBg & 0xFF))
			offset += ctrl & (io::bgTileAddr & 0xFF) ? 0x800 : 0x000;

		//TODO: BG Map, Window map

		printf("%zu %zu %zu %zu\n", ly, ctrl, wy, wx);

		const u16 *tileset = (u16*)(MemoryMapper::mapping | offset);

		for (usz i = 0; i < specs::width; ++i) {

			usz x = i + scx, tx = (x >> 3) & 0x1F;
			u16 tile = tileset[tx + offy];

			tile >>= 2 * (7 - (x & 7));
			tile &= 3;

			//TODO: Remap using bgp

			ppu[i] = palette[tile];

		}

		mem.set<u8>(io::ly, u8(ly + 1));
	}

	//Push blank screen to the immediate buffer
	//Then push the immediate buffer to the screen buffer
	template<bool on>
	_inline_ void pushBlank() {

		u32 *ppu = (u32*)MemoryMapper::ppuStart;

		for (usz i = 0, j = MemoryMapper::ppuLength / 4; i < j; ++i)
			ppu[i] = palette[on ? 0 : 4];

		pushScreen();
	}

	_inline_ usz ppuStep(usz ppuCycle, Emulator::Memory &mem) {

		enum Modes {
			HBLANK = 0,			HBLANK_INTERVAL = 51,
			VBLANK = 1,			VBLANK_INTERVAL = 114,
			OAM = 2,			OAM_INTERVAL = 20,
			VRAM = 3,			VRAM_INTERVAL = 43
		};

		//Push populated frame
		if (mem.get<u8>(io::ctrl) & 0x80) {

			u8 lcdc = mem.get<u8>(io::stat);
			u8 mode = lcdc & 3;

			switch (mode) {

				case HBLANK:

					if (ppuCycle >= HBLANK_INTERVAL) {

						//Push to screen and restart vblank
						if (mem.get<u8>(io::ly) == specs::height) {
							pushScreen();
							goto incrReset;
						}

						//Start scanline
						mode = OAM;
						goto reset;
					}

					break;

				case VBLANK:

					if (ppuCycle >= VBLANK_INTERVAL) {

						if (mem.increment<u8>(io::ly) > 153) {
							mem.set<u8>(io::ly, 0);
							goto incrReset;
						}

						return 0;
					}
					
					break;

				case OAM:

					if (ppuCycle >= OAM_INTERVAL)
						goto incrReset;
					
					break;

				case VRAM:

					if (ppuCycle >= VRAM_INTERVAL) {
						pushLine(mem);
						goto incrReset;
					}

			}

			return ppuCycle + 1;

		incrReset:
			++mode;

		reset:

			mem.set<u8>(io::stat, (lcdc & ~3) | (mode & 3));
			return 0;
		}

		return ppuCycle;
	}

}