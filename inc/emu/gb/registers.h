/* nfsu/gb, a basic gameboy (color) emulator.
*  Copyright (C) 2023 NFSU / Nielsbishere (Niels Brunekreef)
*  
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*  
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*  
*  You should have received a copy of the GNU General Public License
*  along with this program. If not, see https://github.com/Oxsomi/core3/blob/main/LICENSE.
*  Be aware that GPL3 requires closed source products to be GPL3 too if released to the public.
*  To prevent this a separate license will have to be requested at contact@osomi.net for a premium;
*  This is called dual licensing.
*/

#pragma once
#include "psr.h"

typedef union GBRegisters {

	//8-bit registers

	struct {
		GBPSR f;					//Program status register
		U8 a;						//Accumulator for output of math ops
		U8 c, b, e, d, l, h;		//8-bit registers
	};

	U8 reg8[8];

	//16-bit registers

	union {
		U16 af, bc, de, hl;		//16-bit access to 8-bit registers.
		U16 sp;					//Stack pointer
		U16 pc;					//Program counter
	};

	U16 reg16[6];

} GBRegisters;

static const C8 *GBRegisters_reg8[8] = { "F", "A", "C", "B", "E", "D", "L", "H" };
static const C8 *GBRegisters_reg16[6] = { "AF", "BC", "DE", "HL", "SP", "PC" };

static const U8 GBRegisters_instructionReg8[8] = { 3, 2, 5, 4, 7, 6, 8, 1 };		//8 means (HL)
static const U8 GBRegisters_instructionReg16_1[4] = { 1, 2, 3, 0 };					//BC, DE, HL, AF
static const U8 GBRegisters_instructionReg16_2[4] = { 1, 2, 3, 4 };					//BC, DE, HL, SP
