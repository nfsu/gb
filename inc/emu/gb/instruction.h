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
#include "types/types.h"

typedef struct CharString CharString;
typedef struct Error Error;

//The opcode is a Z80 opcode; in total it's technically 9 or so bits.
//16-bit instructions start with 0xCB, giving us a total of ~512 instructions (500 since some are missing)

typedef U16 GBOpcode;

typedef enum EGBOpcodeType EGBOpcodeType;

//Decoded instruction including things such as opcode type and registers it operates on.

typedef struct GBInstruction {

	EGBOpcodeType opcodeType;

	GBOpcode opcode;

	U8 reg;					//Direct index into reg8 or reg16. 8 means (HL), 9 or higher means none.
	U8 reg1;				//Other register. Needs to be 0-7
	U16 intermediate;

	U8 length, cyclesWorst, cyclesBest;

} GBInstruction;

//Stringify for debugging

Error GBInstruction_serialize(GBInstruction instr, CharString *str);
Error GBInstruction_deserialize(CharString str, GBInstruction *instr);
