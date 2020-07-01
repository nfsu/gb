
namespace gb {

	//Debugging

	template<bool isCb, typename ...args>
	_inline_ void Emulator::operation(const args &...arg) {
		oic::System::log()->debug(oic::Log::concat(std::hex, arg...), "\t\t; $", oic::Log::concat(std::hex, pc - 1 - isCb));
	}

	const char *crName[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
	const char *addrRegName[] = { "bc", "de", "hl+", "hl-" };
	const char *shortRegName[] = { "bc", "de", "hl", "sp" };
	const char *shortRegNameAf[] = { "bc", "de", "hl", "af" };
	const char *aluName[] = { "add", "adc", "sub", "sbc", "and", "xor", "or", "cp" };
	const char *condName[] = { "nz", "z", "nc", "c", "" };

	//Function for setting a cr

	template<u8 cr, typename T> _inline_ void Emulator::set(const T &t) {
		m;
		if constexpr (cr != 6) regs[registerMapping[cr]] = t;
		else m.set(hl, t);
	}

	template<u8 c, typename T> _inline_ void Emulator::setc(const T &t) {
		set<(c >> 3) & 7>(t);
	}

	//Function for getting a cr

	template<u8 cr, typename T> _inline_ T Emulator::get() {

		m;

		if constexpr (cr == 6)
			return m.get<T>(hl);

		else if constexpr ((cr & 0x8) != 0) {
			const T t = m.get<T>(pc);
			pc += sizeof(T);
			return t;
		}

		else return regs[registerMapping[cr]];
	}

	template<u8 c, typename T> _inline_ T Emulator::getc() {
		return get<c & 7, T>();
	}

	//Special instructions

	_inline_ usz Emulator::halt() {
		//TODO: HALT
		operation("halt");
		return 1;
	}

	_inline_ usz Emulator::stop() {
		//TODO: Stop
		operation("stop");
		return 1;
	}

	template<bool enable, usz flag>
	_inline_ void Emulator::setFlag() {
		if constexpr (enable)
			m.getMemory<u8>(flag >> 8) |= flag & 0xFF;
		else
			m.getMemory<u8>(flag >> 8) &= ~(flag & 0xFF);
	}

	template<usz flag>
	_inline_ bool Emulator::getFlag() {
		return m.getMemory<u8>(flag >> 8) & (flag & 0xFF);
	}

	template<usz flag>
	_inline_ bool Emulator::getFlagFromAddress() {
		return m.getRef<u8>(flag >> 8) & (flag & 0xFF);
	}

	//LD operations

	//LD cr, cr

	template<u8 c> _inline_ usz Emulator::ld() {

		operation("ld ", crName[(c >> 3) & 7], ",", crName[c & 7]);

		setc<c, u8>(getc<c, u8>());
		return (c & 0x7) == 6 || ((c >> 3) & 7) == 6 ? 2 : 1;
	}

	//LD cr, (pc)

	template<u8 s> _inline_ u16 Emulator::addrFromReg() {

		if constexpr (s < 2)
			return lregs[s];
		else if constexpr (s == 2) {
			++hl;
			return hl - 1;
		} else {
			--hl;
			return hl + 1;
		}

	}

	template<u8 c> _inline_ usz Emulator::ldi() {

		//LD cr, #8
		if constexpr ((c & 7) == 6) {
			u8 v = get<8, u8>();
			operation("ld ", crName[(c >> 3) & 7], ",", u16(v));
			setc<c>(v);
		}

		//LD (nn), A
		else if constexpr ((c & 0xF) == 2) {
			operation("ld (", addrRegName[c >> 4], "),a");
			m[addrFromReg<u8(c >> 4)>()] = a;
		}

		//LD A, (nn)
		else {
			operation("ld a,(", addrRegName[c >> 4], ")");
			a = m[addrFromReg<u8(c >> 4)>()];
		}

		return c == 0x36 ? 3 : 2;
	}

	//LD A, (a16) / LD (a16), A

	template<u8 c> _inline_ usz Emulator::lds() {

		operation("ld ", shortRegName[c / 16], ",", m.get<u16>(pc));

		shortReg<c>() = m.get<u16>(pc);
		pc += 2;

		return 3;
	}

	//All alu operations (ADD/ADC/SUB/SUBC/AND/XOR/OR/CP)

	template<u8 c> _inline_ usz Emulator::performAlu(u8 b) {

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

	template<u8 c> _inline_ usz Emulator::aluOp() {

		//(HL) or (pc)
		if constexpr ((c & 7) == 6) {

			if constexpr (c < 0xC0) {
				operation(aluName[(c >> 3) & 7], " a,(hl)");
				return performAlu<c>(m[hl]);
			}

			else {
				operation(aluName[(c >> 3) & 7], " a,", u16(m[pc]));
				usz v = performAlu<c>(m[pc]);
				++pc;
				return v;
			}
		}

		//Reg ALU
		else {
			operation(aluName[(c >> 3) & 7], " a,", crName[c & 7]);
			return performAlu<c>(regs[c & 7]);
		}
	}

	//RST x instruction

	template<u8 addr> _inline_ usz Emulator::reset() {
		operation("rst ", u16(addr));
		Stack::push(m, sp, pc);
		pc = addr;
		return 4;
	}

	template<u8 c> _inline_ usz Emulator::rst() {
		return reset<(c & 0x30) | (c & 0x8)>();
	}

	//Condtional calls
	template<u8 c> _inline_ bool Emulator::cond() {

		static constexpr u8 condition = (c >> 3) & 3;

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

	template<u8 jp, u8 check> _inline_ usz Emulator::branch() {

		//Print operation

		if constexpr ((jp & 3) == 3)
			operation("ret", jp & 8 ? "i" : " ", check != 0 ? condName[(check >> 3) & 3] : "");

		else if constexpr (inRegion<jp & 3, 1, 3>)
			operation((jp & 3) == 1 ? "jp " : "call ",
					  check != 0 ? condName[(check >> 3) & 3] : "",
					  check != 0 ? "," : "",
					  jp & 4 ? "(hl)" : oic::Log::num<16>(m.get<u16>(pc))
			);

		else if constexpr ((jp & 3) == 0)
			operation("jr ",
					  check != 0 ? condName[(check >> 3) & 3] : "",
					  check != 0 ? "," : "",
					  i32(m.get<i8>(pc))
			);

		//Jump if check failed

		if constexpr (check != 0)
			if (!cond<check>()) {

				//2-width instructions
				if constexpr (inRegion<jp & 3, 1, 3>) {
					pc += 2;
					return 3;
				}

				//1-width instructions
				else if constexpr ((jp & 3) == 0) {
					++pc;
					return 2;
				}

				else return 2;

			}

		//RET
		if constexpr ((jp & 3) == 3) {

			Stack::pop(m, sp, pc);

			if constexpr ((jp & 8) != 0)			//RETI
				setFlag<true, Emulator::IME>();

			return check == 0 ? 4 : 5;
		}

		//JP/CALL
		else if constexpr (inRegion<jp & 3, 1, 3>) {

			//CALL
			if constexpr ((jp & 3) == 2)
				Stack::push(m, sp, pc + 2);

			if constexpr ((jp & 4) != 0)
				pc = m.get<u16>(hl);
			else
				pc = m.get<u16>(pc);

			return 2 + jp * 2;

		}

		//JR
		else {
			pc += u16(i16(m.get<i8>(pc)));
			++pc;
			return 3;
		}
	}

	template<u8 c, u8 code> _inline_ usz Emulator::jmp() {

		if constexpr (c == 0x18)
			return branch<0, 0>();			//JR

		else if constexpr (c < 0x40)
			return branch<0, c>();			//JR conditional

		else if constexpr (c == 0xC3)
			return branch<1, 0>();			//JP

		else if constexpr (code == 2)
			return branch<1, c>();			//JP conditional

		else if constexpr (c == 0xCD)
			return branch<2, 0>();			//CALL

		else if constexpr (code == 4)
			return branch<2, c>();			//CALL conditional

		else if constexpr (c == 0xE9)
			return branch<5, 0>();			//JP (HL)

		else if constexpr (c == 0xC9)
			return branch<7, 0>();			//RET

		else if constexpr (c == 0xD9)
			return branch<15, 0>();			//RETI

		else
			return branch<3, c>();			//RET conditional
	}

	//INC/DEC x instructions

	template<u8 c> _inline_ usz Emulator::inc() {

		static constexpr u8 add = c & 1 ? u8_MAX : 1;
		static constexpr u8 r1 = (c >> 3) & 7;

		if constexpr ((c & 1) == 0) {
			operation("inc ", crName[r1]);
			f.clearSubtract();
		} else {
			operation("dec ", crName[r1]);
			f.setSubtract();
		}

		u8 v;

		if constexpr (r1 != 6)
			v = regs[registerMapping[r1]] += add;

		else
			v = m[hl] += add;

		f.carryHalf(v & 0x10);
		f.zero(v == 0);
		return r1 == 6 ? 3 : 1;
	}

	//Short register functions

	template<u8 c> _inline_ u16 &Emulator::shortReg() {
		if constexpr (((c >> 4) & 3) < 3)
			return lregs[c >> 4];
		else
			return sp;
	}

	template<u8 c> _inline_ usz Emulator::incs() {
		static constexpr u16 add = c & 8 ? u16_MAX : 1;
		operation(c & 8 ? "dec " : "inc ", shortRegName[(c >> 4) & 3]);
		shortReg<c>() += add;
		return 2;
	}

	//Every case runs through this

	template<u8 c, u8 start, u8 end>
	static constexpr bool inRegion = c >= start && c < end;

	template<u8 c, u8 code, u8 hi>
	static constexpr bool isJump =
		(inRegion<c, 0x18, 0x40> && code == 0) ||	//JR
		(inRegion<c, 0xC0, 0xE0> && (				//JP/RET/CALL
									 code == 0 ||							//RET conditional
									 code == 2 ||							//JP conditional
									 code == 4 ||							//CALL conditional
									 c == 0xC3 ||							//JP
									 hi == 9 ||							//RET/RETI
									 c == 0xCD								//CALL
									 )) || c == 0xE9;							//JP (HL)

	enum OpCode : u8 {
		NOP = 0x00, STOP = 0x10, HALT = 0x76,
		DI = 0xF3, EI = 0xFB
	};

	template<u8 c> _inline_ usz Emulator::opCb() {

		static constexpr u8 p = (c >> 3) & 7;
		static constexpr u8 cr = c & 7;

		//Barrel shifts
		if constexpr (c < 0x40) {

			f.clearSubtract();
			f.clearHalf();

			u8 i = get<cr, u8>(), j = i; j;

			//RLC (rotate to the left)

			if constexpr (p == 0) {
				operation("rlc ", crName[cr]);
				a = i = u8(i << 1) | u8(i >> 7);
			}

			//RRC (rotate to the right)

			else if constexpr (p == 1) {
				operation("rrc ", crName[cr]);
				a = i = u8(i >> 1) | u8(i << 7);
			}

			//RL (<<1 shift in carry)

			else if constexpr (p == 2) {
				operation("rl ", crName[cr]);
				a = i = (i << 1) | u8(f.carry());
			}

			//RR (>>1 shift in carry)

			else if constexpr (p == 3) {
				operation("rr ", crName[cr]);
				a = i = (i >> 1) | u8(0x80 * f.carry());
			}

			//SLA (a = cr << 1)

			else if constexpr (p == 4) {
				operation("sla ", crName[cr]);
				a = i <<= 1;
			}

			//SRA (a = cr >> 1 (maintain sign))

			else if constexpr (p == 5) {
				operation("sra ", crName[cr]);
				a = i = (i >> 1) | (i & 0x80);
			}

			//Swap two nibbles

			else if constexpr (p == 6) {
				operation("swap ", crName[cr]);
				a = i = u8(i << 4) | (i >> 4);
			}

			//SRL

			else {
				operation("srl ", crName[cr]);
				a = i >>= 1;
			}

			//Set carry

			if constexpr ((p & 1) == 1)		//Shifted to the left
				f.carry(j & 1);
			else if constexpr (p < 6)		//Shifted to the right
				f.carry(j & 0x80);
			else							//N.A.
				f.clearCarry();

			//Set zero

			f.zero(i == 0);
		}

		//Bit masks

		else if constexpr (c < 0x80) {

			operation<true>("bit ", std::to_string(p), ",", crName[cr]);

			f.setHalf();
			f.clearSubtract();
			f.zero(!(get<cr, u8>() & (1 << p)));
		}

		//Reset bit

		else if constexpr (c < 0xC0) {
			operation<true>("res ", std::to_string(p), ",", crName[cr]);
			set<cr, u8>(get<cr, u8>() & ~(1 << p));
		}

		//Set bit

		else {
			operation<true>("set ", std::to_string(p), ",", crName[cr]);
			set<cr, u8>(get<cr, u8>() | (1 << p));
		}

		return cr == 6 ? 4 : 2;
	}

	template<u8 i> _inline_ usz Emulator::op256() {

		static constexpr u8 code = i & 0x07, hi = i & 0xF;

		if constexpr (i == NOP) {
			operation("nop");
			return 1;
		}

		else if constexpr (i == STOP)
			return stop();

		else if constexpr (i == HALT)
			return halt();

		//DI/EI; disable/enable interrupts
		else if constexpr (i == DI || i == EI) {
			operation(i == DI ? "di" : "ei");
			setFlag<i == EI, Emulator::IME>();
			return 1;
		}

		//LD (a16), SP instruction; TODO:
		else if constexpr (i == 0x8) {
			m.set(m.get<u16>(pc), sp);
			pc += 2;
			return 5;
		}

		//RET, JP, JR and CALL
		else if constexpr (isJump<i, code, hi>)
			return jmp<i, code>();

		//RLCA, RLA, RRCA, RRA
		else if constexpr (i < 0x20 && code == 7)
			return opCb<i>();

		else if constexpr (i < 0x40) {

			if constexpr (hi == 1)
				return lds<i>();

			else if constexpr (hi == 0x9) {

				operation("add hl,", shortRegName[(i >> 4) & 3]);

				u16 hl_ = hl;
				hl += shortReg<i>();

				f.clearSubtract();
				f.carryHalf(hl & 0x10);
				f.carry(hl < hl_);

				return 2;
			}

			else if constexpr (code == 3)
				return incs<i>();

			else if constexpr (code == 4 || code == 5)
				return inc<i>();

			else if constexpr (code == 2 || code == 6)
				return ldi<i>();

			//SCF, CCF

			else if constexpr (i == 0x37 || i == 0x3F) {

				operation(i == 0x37 ? "scf" : "ccf");

				f.clearSubtract();
				f.clearHalf();

				if constexpr (i == 0x37)
					f.setCarry();
				else
					f.carry(!f.carry());

				return 1;
			}

			//CPL

			else if constexpr (i == 0x2F) {
				operation("cpl");
				f.setSubtract();
				f.setHalf();
				a = ~a;
				return 1;
			}

			//DAA

			else if constexpr (i == 0x27) {

				operation("daa");

				u8 k = 0, j = a;
				f.clearCarry();

				if (f.carryHalf() || (!f.subtract() && (j & 0xF) > 0x9))
					k |= 0x6;

				if (f.carry() || (!f.subtract() && j > 0x99)) {
					k |= 0x60;
					f.setCarry();
				}

				a = j += u8(-i8(k));
				f.clearHalf();
				f.zero(j == 0);
				return 1;
			}

			//TODO: LD (a16), SP

			else
				throw std::exception();
		}

		else if constexpr (i < 0x80)
			return ld<i>();

		else if constexpr (i < 0xC0 || code == 0x6)
			return aluOp<i>();

		else if constexpr (code == 0x7)
			return rst<i>();

		//LD (a8/a16/C)/A, (a8/a16/C)/A
		else if constexpr (i >= 0xE0 && (hi == 0x0 || hi == 0x2 || hi == 0xA)) {

			u16 addr;

			//TODO: ???

			if constexpr (hi == 0x0) {

				const u8 im = m[pc];

				operation(
					"ldh ",
					i < 0xF0 ? oic::Log::num<16>((u16)im) : "a", ",",
					i >= 0xF0 ? oic::Log::num<16>((u16)im) : "a"
				);

				addr = 0xFF00 | im;
				++pc;
			}

			else if constexpr (hi == 0x2) {
				operation("ld ", i < 0xF0 ? "(c)," : "a,", i >= 0xF0 ? "(c)" : "a");
				addr = 0xFF00 | c;
			}

			else {

				const u16 j = m.get<u16>(pc);

				operation(
					"ld ",
					i < 0xF0 ? oic::Log::num<16>(j) : "a", ",",
					i >= 0xF0 ? oic::Log::num<16>(j) : "a"
				);

				addr = j;
				pc += 2;
			}

			if constexpr (i < 0xF0)		//Store
				m[addr] = a;
			else						//Load
				a = m[addr];

			return 1;
		}

		//CB instructions (mask, reset, set, rlc/rrc/rl/rr/sla/sra/swap/srl)
		else if constexpr (i == 0xCB)
			return cbInstruction();

		//PUSH/POP
		else if constexpr (hi == 1 || hi == 5) {

			static constexpr u8 reg = (i - 0xC0) >> 4;

			operation(hi == 1 ? "pop " : "push ", shortRegNameAf[reg]);

			//Push to or pop from stack
			if constexpr (hi == 1)
				Stack::pop(m, sp, lregs[reg]);
			else
				Stack::push(m, sp, lregs[reg]);

			return hi == 1 ? 3 : 4;
		}

		//LD HL, SP+a8
		else if constexpr (i == 0xF8) {

			operation("ld hl, sp+", u16(m[pc]));

			hl = emu::add(f, sp, u16(m[pc]));
			++pc;

			f.clearZero();

			return 3;
		}

		//TODO: ADD SP, r8
		//TODO: LD HL, SP+a8
		//TODO: LD SP, HL

		else										//Undefined operation
			throw std::exception();
	}

	//Switch case of all opcodes

	//For our switch case of 256 entries
	#define case1(x,y) case x: return op##y<x>();
	#define case2(x,y) case1(x,y) case1(x + 1,y)
	#define case4(x,y) case2(x,y) case2(x + 2,y)
	#define case8(x,y) case4(x,y) case4(x + 4,y)
	#define case16(x,y) case8(x,y) case8(x + 8,y)
	#define case32(x,y) case16(x,y) case16(x + 16,y)
	#define case64(x,y) case32(x,y) case32(x + 32,y)
	#define case128(x,y) case64(x,y) case64(x + 64,y)
	#define case256(y) case128(0,y) case128(128,y)

	_inline_ usz Emulator::cbInstruction() {

		u8 opCode = m[pc];
		print(u16(opCode), '\t');
		++pc;

		switch (opCode) {
			case256(Cb)
		}

		return 0;
	}

	_inline_ usz Emulator::cpuStep() {
		
		u8 &mem = m.getMemory<u8>(Emulator::IS_IN_BIOS >> 8);
		constexpr u8 bit = Emulator::IS_IN_BIOS & 0xFF;

		if (mem & bit && pc >= MemoryMapper::biosLength)
			mem &= ~bit;

		u8 opCode = m[pc];
		print(std::hex, pc, ' ', u16(opCode), '\t');
		++pc;

		switch (opCode) {
			case256(256)
		}

		return 0;
	}

	_inline usz Emulator::interruptHandler() {

		#ifndef NDEBUG

		print(
			u16(a), " ", u16(b), " ",
			u16(c), " ", u16(d), " ",
			u16(e), " ", u16(f.v), " ",
			sp, "\n"
		);

		/*if((pc >= 0x2A0 && pc < 0x2795) || pc >= 0x27A3)
			system("pause");*/
		#endif

		usz counter {};

		if (getFlag<Emulator::IME>()) {

			u8 &u = m.getRef<u8>(io::IF);
			const u8 &enabled = m.getRef<u8>(io::IE);

			const u8 g = u & enabled;

			u &= ~g;	//Mark as handled

			if (g)
				setFlag<false, Emulator::IME>();

			if (g & 1)               //V-Blank
				counter += reset<0x40>();

			if (g & 2)               //LCD STAT
				counter += reset<0x48>();

			if (g & 4)               //Timer
				counter += reset<0x50>();

			if (g & 8)               //Serial
				counter += reset<0x58>();

			if (g & 16)              //Joypad
				counter += reset<0x60>();

		}

		return counter;
	}

}