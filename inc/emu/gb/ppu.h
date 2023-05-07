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

typedef struct GBMMU GBMMU;

typedef struct GBPPU {

	U8 palette[5][3];
	U32 screen[144][160];

} GBPPU;

static const U8 GBPPU_defaultPalette[5][3] = {
	{ 136, 148, 87 },
	{  84, 118, 89 },
	{  59, 88,  76 },
	{  34, 58,  50 },
	{  66, 81,  3 }
};

void GBPPU_setPaletteColor(GBPPU *ppu, U32 rgb);
void GBPPU_pushLine(GBPPU *ppu, const GBMMU *mmu);
void GBPPU_clear(GBPPU *ppu);
void GBPPU_step(GBPPU *ppu, const GBMMU *mmu);
