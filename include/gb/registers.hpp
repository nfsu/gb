#pragma once
#include "psr.hpp"

namespace gb {

	union Registers {

		//CR mapping
		//B,C, D,E, H,L, (HL),A
		//(HL) should be handled by the instruction itself since it uses the memory model
		static constexpr usz mapping[] = {
			1,0, 3,2, 5,4, usz_MAX,7 
		};

		//8-bit registers
		struct {
			u8 c, b, e, d, l, h;
			PSR f;
			u8 a; 
		};

		//8-bit registers accessed as 16-bit
		//and stack pointer & program counter
		struct {
			u16 bc, de, hl, af, sp, pc;
		};

		struct {
			u8 regs[8];
		};

		struct {
			u16 lregs[6];
		};

		Registers(): af(), bc(), de(), hl(), sp(), pc() {}

	};

}