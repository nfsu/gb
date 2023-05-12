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
#include "platforms/log.h"

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

Bool GBEmulator_pushStack8(GBEmulator *emu, U16 v) {
	return GBMMU_setU8(emu->memory, emu->registers.sp--, v);
}

Bool GBEmulator_pushStack16(GBEmulator *emu, U16 v) {
	return GBEmulator_pushStack8(emu, (U8)(v >> 8)) && GBEmulator_pushStack8(emu, (U8)v);
}

Bool GBEmulator_execute(GBEmulator *emu, GBInstruction i) {

	if(!emu)
		return false;

	GBRegisters *r = &emu->registers;
	GBMMU *mem = emu->memory;

	//Instruction was read, increment program counter

	r->pc += i.length;

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

		//LDH (a8),A and LD (C),A (and reverse) 
		//are both considered LDHA type.

		case LDHA: {

			//Load A register, (0xFF00 + C or i)

			U8 res;

			if(i.reg1 == 1)
				res = r->a;

			else if(i.reg1 == 3) {
				if(!GBMMU_getU8(mem, 0xFF00 + r->c, &res, NULL))
					return false;
			}

			else if(!GBMMU_getU8(mem, 0xFF00 + i.intermediate, &res, NULL))
				return false;

			//Store into A, (0xFF00 + C or i)

			if(i.reg == 1)
				r->a = res;

			else if(i.reg == 3) {
				if(!GBMMU_setU8(mem, 0xFF00 + r->c, res))
					return false;
			}

			else if(!GBMMU_setU8(mem, 0xFF00 + i.intermediate, res))
				return false;
		
			break;
		}

		//LD (a16),A and reverse.

		case LDA16: {

			//Load

			U8 v = r->a;

			if(i.reg1 != 1 && GBMMU_getU8(mem, i.intermediate, &v, NULL))
				return false;

			//Store

			if(i.reg == 1)
				r->a = v;

			else if(!GBMMU_setU8(mem, i.intermediate, v))
				return false;

			break;
		}

		//LD 8-bit constant

		case LD8_TO_REG: {

			if(i.reg < 8)
				r->reg8[i.reg] = (U8) i.intermediate;

			else if(!GBMMU_setU8(mem, r->hl, i.intermediate))
				return false;

			break;
		}

		//LD 16-bit constant

		case LD16_TO_REG:
			r->reg16[i.reg] = i.intermediate;
			break;

		//Reset pushes PC onto the stack and redirects to i.intermediate

		case RST:
			GBEmulator_pushStack16(emu, r->pc);
			r->pc = i.intermediate;
			break;

		//TODO: LD SP,HL and LD HL,SP+r8
		//TODO: LD (r16+-),d16
		//TODO: LD (a16),SP

		//ALU

		case SBC:
		case SUB: 
		case ADC:
		case ADD: {

			U8 t = (U8)i.intermediate, prev = r->a;

			if(i.reg && !GBEmulator_getU8(emu, i.reg, &t))
				return false;

			Bool isAdc = i.opcodeType == ADC, isAdd = isAdc || i.opcodeType == ADD;
			Bool carry = (isAdc || i.opcodeType == SBC) && GBPSR_carry(r->f);

			U16 v = isAdd ? (U16)r->a + t + carry : r->a - t - carry;
			r->a = (U8)v;

			Bool H = !isAdd ? (prev & 16) && !(r->a & 16) : (prev & 8) && !(v & 8);
			GBPSR_setBits(&r->f, !r->a, !isAdd, H, v > U8_MAX);

			break;
		}

		case OR:
		case AND:
		case XOR: {

			U8 t = (U8)i.intermediate;

			if(i.reg && !GBEmulator_getU8(emu, i.reg, &t))
				return false;

			if(i.opcodeType == OR)
				r->a |= t;

			else if(i.opcodeType == XOR)
				r->a ^= t;

			else r->a &= t;
			
			GBPSR_setBits(&r->f, !r->a, 0, i.opcodeType == AND, 0);
			break;
		}

		case INC:
		case DEC: {

			U8 prev = i.intermediate;

			if(!GBEmulator_getU8(emu, i.reg, &prev))
				return false;

			U8 res = i.opcodeType == DEC ? prev - 1 : prev + 1;

			if(!GBEmulator_setU8(emu, i.reg, res))
				return false;
			
			Bool H = i.opcodeType == DEC ? (prev & 16) && !(res & 16) : (prev & 8) && !(res & 8);
			GBPSR_setBits(&r->f, !res, i.opcodeType == DEC, H, -1);
			break;
		}

		case CP: {

			//TODO: Compare

			break;
		}

		case DAA:	break;		//TODO: 
		case SCF:	break;		//TODO: 
		case CPL:	break;		//TODO: 
		case CCF:	break;		//TODO: 

		case INC16:
			++r->reg16[i.reg];
			break;

		case DEC16: 
			--r->reg16[i.reg];
			break;

		case ADD16:	break;		//TODO:

		//Bitwise operators

		//Bit operation sets N to 0, H to 1 and Z to the bit.

		case BIT: {

			U8 res;

			if(!GBEmulator_getU8(emu, i.reg, &res))
				return false;

			GBPSR_setBits(&r->f, (res >> i.intermediate) & 1, 0, 1, -1);
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
		//case SLA, SRA, SWAP, SRL

		//Rotate carry into left or right bit,
		//Or rotate self left/right.

		case RL:
		case RR: 
		case RRC:
		case RLC: {

			U8 t;

			if(!GBEmulator_getU8(emu, i.reg, &t))
				return false;

			Bool isLeft = i.opcodeType & 1;
			Bool C = t & (isLeft ? 0x80 : 0x01);
			U8 newBit = GBPSR_carry(r->f);

			if(i.opcodeType < RL)
				newBit = isLeft ? t >> 7 : t & 1;

			U8 v = isLeft ? (t << 1) | newBit : (t >> 1) | (newBit << 7);

			if(!GBEmulator_setU8(emu, i.reg, v))
				return false;

			GBPSR_setBits(&r->f, !v, 0, 0, C);
			break;
		}

		//Shift right and left using 0 bit.
		//As well as signed right shift (preserves carry).

		case SRL:
		case SRA:
		case SLA: {
			//TODO:
			break;
		}

		//Swap two nibbles

		case SWAP: {

			U8 t;

			if(!GBEmulator_getU8(emu, i.reg, &t))
				return false;

			if(!GBEmulator_setU8(emu, (t << 4) | (t >> 4), &t))
				return false;

			GBPSR_setBits(&r->f, !t, 0, 0, 0);
			break;
		}

		//Special instructions

		case STOP:
			//TODO:
			break;

		//Halt until interrupt

		case HALT:
			//TODO:
			break;

		//Disable and enable interrupts

		case DI:
			//TODO:
			break;

		case EI:
			//TODO:
			break;

		//Nothing

		case NOP:
			break;

		default:
			Log_errorLn("Unsupported instruction in execution!");
			return false;
	}

	//Add cycles so PPU can sync

	emu->cycles += didExec ? i.cyclesWorst : i.cyclesBest;

	return true;
}