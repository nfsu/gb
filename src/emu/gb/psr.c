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

#include "gb/psr.h"

Bool GBPSR_carryHalf(GBPSR psr) { return psr & GBPSR_halfCarryMask; }
Bool GBPSR_carry(GBPSR psr) { return psr & GBPSR_carryMask; }
Bool GBPSR_zero(GBPSR psr) { return psr & GBPSR_zeroMask; }
Bool GBPSR_subtract(GBPSR psr)  { return psr & GBPSR_subtractMask; }

void GBPSR_setCarryHalf(GBPSR *psr) { if(psr) *psr |= GBPSR_halfCarryMask; }
void GBPSR_setCarry(GBPSR *psr) { if(psr) *psr |= GBPSR_carryMask; }
void GBPSR_setZero(GBPSR *psr) { if(psr) *psr |= GBPSR_zeroMask; }
void GBPSR_setSubtract(GBPSR *psr) { if(psr) *psr |= GBPSR_subtractMask; }

void GBPSR_resetCarryHalf(GBPSR *psr) { if(psr) *psr &= ~GBPSR_halfCarryMask; }
void GBPSR_resetCarry(GBPSR *psr) { if(psr) *psr &= ~GBPSR_carryMask; }
void GBPSR_resetZero(GBPSR *psr) { if(psr) *psr &= ~GBPSR_zeroMask; }
void GBPSR_resetSubtract(GBPSR *psr) { if(psr) *psr &= ~GBPSR_subtractMask; }

void GBPSR_setCarryHalfTo(GBPSR *psr, Bool b) { b ? GBPSR_setCarryHalf(psr) : GBPSR_resetCarryHalf(psr); }
void GBPSR_setCarryTo(GBPSR *psr, Bool b) { b ? GBPSR_setCarry(psr) : GBPSR_resetCarry(psr); }
void GBPSR_setZeroTo(GBPSR *psr, Bool b) { b ? GBPSR_setZero(psr) : GBPSR_resetZero(psr); }
void GBPSR_setSubtractTo(GBPSR *psr, Bool b) { b ? GBPSR_setSubtract(psr) : GBPSR_resetSubtract(psr); }

void GBPSR_setBits(GBPSR *psr, I8 zero, I8 subtract, I8 half, I8 carry) {

	if(!psr)
		return;

	if(zero >= 0)
		GBPSR_setZeroTo(psr, (Bool) zero);

	if(subtract >= 0)
		GBPSR_setSubtractTo(psr, (Bool) subtract);

	if(half >= 0)
		GBPSR_setCarryHalfTo(psr, (Bool) half);

	if(carry >= 0)
		GBPSR_setCarryTo(psr, (Bool) carry);
}
