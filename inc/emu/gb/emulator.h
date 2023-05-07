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
#include "registers.h"
#include "ppu.h"

typedef struct GBInstruction GBInstruction;
typedef struct GBMMU GBMMU;

typedef struct GBEmulator {

	GBRegisters registers;
	GBMMU *memory;
	GBPPU ppu;

	U32 cycles;

} GBEmulator;

Error GBEmulator_create(GBEmulator **emulator);
Bool GBEmulator_free(GBEmulator **emulator);

Bool GBEmulator_setBios(GBEmulator *emulator, Buffer bios);
Bool GBEmulator_setRom(GBEmulator *emulator, Buffer bios);

GBInstruction GBEmulator_decode(const GBEmulator *emulator, U16 addr);
Error GBEmulator_printInstructionAt(const GBEmulator *emulator, U16 addr);

Error GBEmulator_stringifyState(const GBEmulator *emulator, CharString *str);
Error GBEmulator_printState(const GBEmulator *emulator);

Bool GBEmulator_execute(GBEmulator *emulator, GBInstruction instruction);
Bool GBEmulator_step(GBEmulator *emulator, GBInstruction *instruction);
