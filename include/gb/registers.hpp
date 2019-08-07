#pragma once
#include "psr.hpp"

namespace gb {

	union Registers {

		//8-bit registers
		struct {
			u8 b, c, d, e, h, l;
			PSR f;
			u8 a; 
		};

		//8-bit registers accessed as 16-bit
		//and stack pointer & program counter
		struct {
			u16 bc, de, hl, fa, sp, pc;
		};

		struct {
			u8 regs[8];
		};

		Registers(): fa(), bc(), de(), hl(), sp(0xFFFE), pc(0x100) {}

	};

}