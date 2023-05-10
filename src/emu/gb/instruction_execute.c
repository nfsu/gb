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

#include "gb/emulator.h"
#include "gb/instruction.h"
#include "gb/opcode_type.h"
#include "gb/mmu.h"

Bool GBEmulator_getU8(GBEmulator *emu, U8 reg, U8 *res) {

	if(reg != 8)
		res = emu->registers.reg8[reg];

	else if(!GBMMU_getU8(emu->memory, emu->registers.hl, &res, NULL))
		return false;

	return true;
}

Bool GBEmulator_setU8(GBEmulator *emu, U8 reg, U8 v) {

	if(reg != 8)
		emu->registers.reg8[reg] = v;

	else if(!GBMMU_setU8(emu->memory, emu->registers.hl, v))
		return false;

	return true;
}

Bool GBEmulator_execute(GBEmulator *emu, GBInstruction i) {

	if(!emu)
		return false;

	//Instruction was read, increment program counter

	emu->registers.pc += i.length;

	//Jump instructions have different timings if executed, they set this.

	Bool didExec = true;

	//Execute instruction

	switch (i.opcodeType) {

		//Loads are simple since they don't affect the PSR

		//LD can load from any register into any other register
		//Special case is (HL) which needs to load from the address stored in HL.
		
		case LD_REG_TO_REG: {

			U8 res;

			if(!GBEmulator_getU8(emu, i.reg1, &res))
				return false;

			if(!GBEmulator_setU8(emu, i.reg, &res))
				return false;

			break;
		}

		//TODO: ALU

		//Bitwise operators

		//Bit operation sets N to 0, H to 1 and Z to the bit.

		case BIT: {

			U8 res;

			if(!GBEmulator_getU8(emu, i.reg, &res))
				return false;

			GBPSR_setBits(emu->registers.f, (res >> i.intermediate) & 1, 0, 1, -1);
			break;
		}

		//Reset clears the bit, SET sets it

		case SET:
		case RES: {

			U8 res;

			if(!GBEmulator_getU8(emu, i.reg, &res))
				return false;

			if(i.opcodeType == RES)
				res &= ~(1 << i.intermediate);

			else res |= (1 << i.intermediate);

			if(!GBEmulator_setU8(emu, i.reg, &res))
				return false;

			break;	
		}

		//TODO:
		//case RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL

	}

	//Add cycles so PPU can sync

	emu->cycles += didExec ? i.cyclesWorst : i.cyclesBest;

	return true;
}