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

typedef struct Error Error;

typedef struct GBMMU {

	U8 bios[0x100];
	U8 rom[0x200000];
	U8 VRAM[0x2000];
	U8 xRAM[0x2000];
	U8 RAM[0x2000];
	U8 spriteInfo[0xA0];
	U8 controlValues[0x80];
	U8 zeroPage[0x80];
	U8 mmu[0x20];

	Bool hasRom, hasBios;
	Bool booted;

} GBMMU;

Error GBMMU_create(GBMMU **mem);
Bool GBMMU_free(GBMMU **mem);

Bool GBMMU_setBios(GBMMU *mem, Buffer bios);
Bool GBMMU_setRom(GBMMU *mem, Buffer rom);

Bool GBMMU_setU8(GBMMU *mem, U16 addr, U8 v, U8 *cycleCounter);
Bool GBMMU_setU16(GBMMU *mem, U16 addr, U16 v, U8 *cycleCounter);

Bool GBMMU_getU8(const GBMMU *mem, U16 addr, U8 *v, U8 *cycleCounter);
Bool GBMMU_getU16(const GBMMU *mem, U16 addr, U16 *v, U8 *cycleCounter);
