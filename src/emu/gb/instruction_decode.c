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

		//TODO: Stop is suffixed with 0x00

		case 0x10:	result.opcodeType = STOP;	break;

		//Return (unconditional)

		case 0xD9:
			result.opcodeType = RETI;
			cycles += 3; 
			break;

		case 0xC9:
			result.opcodeType = RET;
			cycles += 3;
			result.reg1 = 4;
			break;

		//LD (a16),SP

		case 0x08:

			if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
				return result;

			addr += 2;
			cycles += 2;

			result.opcodeType = LD_SP;
			break;

		//Jump (unconditional)

		case 0xC3:

			if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
				return result;

			addr += 2;

			++cycles;
			result.opcodeType = JP;
			result.reg1 = 4;
			break;

		//Call

		case 0xCD:	

			if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
				return result;

			addr += 2;

			result.opcodeType = CALL;
			cycles += 3; 
			break;

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
					result.intermediate = (vlo - 0xC0) >> 3;
					result.opcodeType = SET;
				}

				//Reset

				else {
					result.intermediate = (vlo - 0x80) >> 3;
					result.opcodeType = RES;
				}
			}

			//Partition lower

			else {

				//Get bit

				if (vlo >= 0x40) {
					result.intermediate = (vlo - 0x40) >> 3;
					result.opcodeType = BIT;
				}

				//Single register operations

				else {

					if(vlo >= 0x20) {

						if(vlo >= 0x30)
							result.opcodeType = vlo >= 0x38 ? SRL : SWAP;

						else result.opcodeType = vlo >= 0x28 ? SRA : SLA;
					}

					else {

						if(vlo >= 0x10)
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

				U8 subType = v & 7;

				//Push, pop and most jumps

				if (v >= 0xC0 && (subType != 6)) {

					switch (subType) {

						//Conditional return

						case 0: {

							++cycles;
							cycleMaxDelta = 3;

							switch (v & 0x18) {
								case 0x00:	result.reg1 = 2;	break;
								case 0x08:	result.reg1 = 0;	break;
								case 0x10:	result.reg1 = 1;	break;
								case 0x18:	result.reg1 = 3;	break;
							}

							result.opcodeType = RET;
							break;
						}

						//POP

						case 1:
							result.opcodeType = POP;
							result.reg = GBRegisters_instructionReg16_1[(v - 0xC0) >> 4];
							cycles += 2;
							break;

						//Jump

						case 2:

							if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
								return result;

							addr += 2;
						
							switch (v & 0x18) {
								case 0x00:	result.reg1 = 2;	break;
								case 0x08:	result.reg1 = 0;	break;
								case 0x10:	result.reg1 = 1;	break;
								case 0x18:	result.reg1 = 3;	break;
							}

							cycleMaxDelta = 1;
							result.opcodeType = JP;
							break;

						//Already handled explicitly

						case 3: break;

						//Call

						case 4:

							if(!GBMMU_getU16(emulator->memory, addr, &result.intermediate, &cycles))
								return result;

							addr += 2;
						
							switch (v & 0x18) {
								case 0x00:	result.reg1 = 2;	break;
								case 0x08:	result.reg1 = 0;	break;
								case 0x10:	result.reg1 = 1;	break;
								case 0x18:	result.reg1 = 3;	break;
							}

							cycleMaxDelta = 3;
							result.opcodeType = CALL;
							break;

						//PUSH

						case 5:
							result.opcodeType = PUSH;
							result.reg = GBRegisters_instructionReg16_1[(v - 0xC0) >> 4];
							cycles += 3;
							break;

						//Handled by ALU branch

						case 6: break;

						//RST

						case 7:
							result.opcodeType = RST;
							result.intermediate = ((v - 0xC0) & 0x30) | (v & 8);
							cycles += 3;
							break;
					}
				}

				//ALU

				else {

					U8 rel = v & 0x3F;
					Bool isIntermediate = v >= 0xC0;

					//AND, XOR, OR, CP

					if (rel >= 0x20) {

						result.reg = GBRegisters_instructionReg8[v & 7];

						if(rel >= 0x30)
							result.opcodeType = rel >= 0x38 ? CP : OR;

						else result.opcodeType = rel >= 0x28 ? XOR : AND;
					}

					//ADD, ADC, SUB, SBC

					else {

						result.reg = 1;
						result.reg1 = GBRegisters_instructionReg8[v & 7];

						//SUB, SBC

						if(rel >= 0x10) {

							if(rel >= 0x18)
								result.opcodeType = SBC;
							
							else {
								result.opcodeType = SUB;
								result.reg = result.reg1;
								result.reg1 = 0;
							}
						}

						//ADD, ADC

						else result.opcodeType = rel >= 0x08 ? ADC : ADD;
					}

					//If intermediate, we have no register we work on.

					if (isIntermediate) {

						//Intermediate stored next to the instruction

						if(!GBMMU_getU8(emulator->memory, addr++, (U8*) &result.intermediate, &cycles))
							return result;

						result.reg = result.reg1 ? 1 : 0;
						result.reg1 = 0;
					}

					//(HL) will incur another read, which takes 1 m cycle.

					else if(result.reg == 8 || result.reg1 == 8)
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

								switch (v & 0x18) {
									case 0x00:	result.reg1 = 2;	break;
									case 0x08:	result.reg1 = 0;	break;
									case 0x10:	result.reg1 = 1;	break;
									case 0x18:	result.reg1 = 3;	break;
								}
							}
						
							result.opcodeType = JR;
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

							result.reg = U8_min((U8)(1 + (v >> 4)), 3);
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
	result.length = (U8)(addr - prevAddr);
	result.cyclesBest = cycles;							//M cycles
	result.cyclesWorst = cycles + cycleMaxDelta;		//M cycles
	return result;
}
