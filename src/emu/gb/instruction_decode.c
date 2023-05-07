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
#include "gb/mmu.h"
#include "gb/instruction.h"
#include "gb/opcode_type.h"
#include "math/math.h"

GBInstruction GBEmulator_decode(const GBEmulator *emulator, U16 addr) {

	GBInstruction result = (GBInstruction) { 0 };

	if(!emulator)
		return result;

	//Get instruction start

	U8 cycles = 0, cycleMaxDelta = 0;
	U16 v = 0, prevAddr = addr;
	
	if(!GBMMU_getU8(emulator->memory, addr++, (U8*) &v, &cycles))
		return result;

	//Decode

	switch (v) {

		//One off instructions; don't follow any sort of pattern.

		case 0x00:	result.opcodeType = NOP;	break;
		case 0x76:	result.opcodeType = HALT;	break;
		case 0xF3:	result.opcodeType = DI;		break;
		case 0xFB:	result.opcodeType = EI;		break;

		case 0x07:	result.opcodeType = RLCA;	break;
		case 0x0F:	result.opcodeType = RRCA;	break;
		case 0x17:	result.opcodeType = RLA;	break;
		case 0x1F:	result.opcodeType = RRA;	break;

		case 0x27:	result.opcodeType = DAA;	break;
		case 0x2F:	result.opcodeType = CPL;	break;
		case 0x37:	result.opcodeType = SCF;	break;
		case 0x3F:	result.opcodeType = CCF;	break;

		case 0xD9:	result.opcodeType = RETI;	break;

		//TODO: Stop is suffixed with 0x00

		case 0x10:	result.opcodeType = STOP;	break;

		//Extended instruction

		case 0xCB: {

			v <<= 8;

			if(!GBMMU_getU8(emulator->memory, addr++, (U8*) &v, &cycles))
				return result;

			U8 vlo = (U8) v;

			result.reg = GBRegisters_instructionReg8[v & 7];

			//Use some partitioning to reduce checks

			if(vlo >= 0x80) {

				//Set

				if (vlo >= 0xC0) {
					result.intermediate = (v - 0xC0) >> 3;
					result.opcodeType = SET;
				}

				//Reset

				else {
					result.intermediate = (v - 0x80) >> 3;
					result.opcodeType = RES;
				}
			}

			//Partition lower

			else {

				//Get bit

				if (vlo >= 0x40) {
					result.intermediate = (v - 0x40) >> 3;
					result.opcodeType = BIT;
				}

				//Single register operations

				else {

					if(vlo >= 0x20) {

						if(v >= 0x30)
							result.opcodeType = vlo >= 0x38 ? SRL : SWAP;

						else result.opcodeType = vlo >= 0x28 ? SRA : SLA;
					}

					else {

						if(v >= 0x10)
							result.opcodeType = vlo >= 0x18 ? RR : RL;

						else result.opcodeType = vlo >= 0x08 ? RRC : RLC;
					}
				}
			}

			break;
		}

		//Instructions that follow certain patterns
			
		default: {

			//High side

			if (v >= 0x80) {

				//Push, pop, most jumps and some alu

				if (v >= 0xC0) {
					//TODO:
				}

				//ALU

				else {

					//AND, XOR, OR, CP

					if (v >= 0xA0) {

						result.reg = GBRegisters_instructionReg8[v & 7];

						if(v >= 0xB0)
							result.opcodeType = v >= 0xB8 ? CP : OR;

						else result.opcodeType = v >= 0xA8 ? XOR : AND;
					}

					//ADD, ADC, SUB, SBC

					else {

						result.reg = 1;
						result.reg1 = GBRegisters_instructionReg8[v & 7];

						//SUB, SBC

						if(v >= 0x90) {

							if(v >= 0x98)
								result.opcodeType = SBC;
							
							else {
								result.opcodeType = SUB;
								result.reg = result.reg1;
								result.reg1 = 0;
							}
						}

						//ADD, ADC

						else result.opcodeType = v >= 0x88 ? ADC : ADD;
					}

					//(HL) will incur another read, which takes 1 m cycle.

					if(result.reg == 8 || result.reg1 == 8)
						++cycles;
				}
			}

			//Low side

			else {

				//Load register to register

				if (v >= 0x40) {

					U8 reg = GBRegisters_instructionReg8[(v - 0x40) >> 3];
					U8 reg1 = GBRegisters_instructionReg8[v & 7];

					result.opcodeType = LD_REG_TO_REG;
					result.reg = reg;
					result.reg1 = reg1;

					//(HL) will incur another read, which takes 1 m cycle.

					if(reg == 8 || reg1 == 8)
						++cycles;

					break;
				}

				//Other instructions.
				//These follow a pattern in pairs

				else {

					U8 reg8 = GBRegisters_instructionReg8[((v >> 4) << 1) | ((v & 0xF) >= 0x8)];

					switch (v & 7) {

						//Relative jump

						case 0:

							//Relative address stored next to the instruction

							if(!GBMMU_getU8(emulator->memory, addr++, (U8*) &result.intermediate, &cycles))
								return result;

							//JR always executes.
							//This means our cycle max delta is 0, 
							//since the next instruction will always be fetched

							if(v == 0x18) {
								++cycles;
								cycleMaxDelta = 0;
								result.reg1 = 4;
							}

							//Conditional relative jumps.
							//This means the next m cycle hit isn't guaranteed.
							//The execution will pick between best and worst cycles,
							//depending on whether or not the condition is satisfied.

							else {

								cycleMaxDelta = 1;

								switch (v) {
									case 0x20:	result.reg1 = 2;	break;
									case 0x28:	result.reg1 = 0;	break;
									case 0x30:	result.reg1 = 1;	break;
									case 0x38:	result.reg1 = 3;	break;
								}
							}
						
							break;

						//LD 16-bit register and ADD

						case 1:

							result.reg = GBRegisters_instructionReg16_2[v >> 4];

							//ADD

							if ((v & 0xF) >= 0x8) {
								result.opcodeType = ADD16_HL;
								++cycles;
							}

							//LD 16-bit reg
							
							else {

								if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
									return result;

								addr += 2;
								result.opcodeType = LD16_TO_REG;
							}

							break;

						//LD from address and to address with A

						case 2: {

							result.reg = U8_min(1 + (v >> 4), 3);
							result.reg1 = 1;
							result.opcodeType = LD_A_TO_ADDR;

							if ((v & 0xF) >= 0x8) {
								result.reg1 = result.reg;
								result.reg = 1;
								result.opcodeType = LD_A_FROM_ADDR;
							}

							result.intermediate = (U8) I8_max((I8)(v >> 4) - 1, 0);
							++cycles;

							break;
						}

						//INC and DEC 16-bit register

						case 3:

							result.reg = GBRegisters_instructionReg16_2[v >> 4];
							++cycles;

							result.opcodeType = (v & 0xF) >= 0x8 ? DEC : INC;
							break;

						//INC/DEC 8-bit register

						case 4:
						case 5:

							result.opcodeType = (v & 7) == 4 ? INC : DEC;
							result.reg = reg8;

							//(HL) will incur another read, which takes 2 m cycles.
							//One to read, one to write.

							if(result.reg == 8)
								cycles += 2;

							break;

						//LD intermediate 

						case 6:

							result.opcodeType = LD8_TO_REG;
							result.reg = reg8;

							//Intermediate is stored next to instruction

							if(!GBMMU_getU8(emulator->memory, addr++, (U8*) &result.intermediate, &cycles))
								return result;

							//(HL) will incur another read, which takes 1 m cycle.

							if(result.reg == 8)
								++cycles;

							break;

						//Shifts and dec operations are manually handled in op code switch.

						case 7: break;
					}
				}
			}

			break;
		}
	}

	//Assign length and timings

	result.opcode = v;
	result.length = addr - prevAddr;
	result.cyclesBest = cycles;							//M cycles
	result.cyclesWorst = cycles + cycleMaxDelta;		//M cycles
	return result;
}
