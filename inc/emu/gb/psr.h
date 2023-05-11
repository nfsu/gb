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

typedef U8 GBPSR;

static const GBPSR GBPSR_zeroMask		= 0x80;
static const GBPSR GBPSR_subtractMask	= 0x40;
static const GBPSR GBPSR_halfCarryMask	= 0x20;
static const GBPSR GBPSR_carryMask		= 0x10;

Bool GBPSR_carryHalf(GBPSR psr);
Bool GBPSR_carry(GBPSR psr);
Bool GBPSR_zero(GBPSR psr);
Bool GBPSR_subtract(GBPSR psr);

void GBPSR_setCarryHalf(GBPSR *psr);
void GBPSR_setCarry(GBPSR *psr);
void GBPSR_setZero(GBPSR *psr);
void GBPSR_setSubtract(GBPSR *psr);

void GBPSR_resetCarryHalf(GBPSR *psr);
void GBPSR_resetCarry(GBPSR *psr);
void GBPSR_resetZero(GBPSR *psr);
void GBPSR_resetSubtract(GBPSR *psr);

void GBPSR_setCarryHalfTo(GBPSR *psr, Bool b);
void GBPSR_setCarryTo(GBPSR *psr, Bool b);
void GBPSR_setZeroTo(GBPSR *psr, Bool b);
void GBPSR_setSubtractTo(GBPSR *psr, Bool b);

void GBPSR_setBits(GBPSR *psr, I8 zero, I8 subtract, I8 half, I8 carry);
