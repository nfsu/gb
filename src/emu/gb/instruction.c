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

#include "gb/instruction.h"
#include "gb/registers.h"
#include "gb/opcode_type.h"
#include "platforms/ext/stringx.h"
#include "platforms/ext/errorx.h"

Error GBInstruction_serialize(GBInstruction instr, CharString *str) {

	if(!str)
		return Error_nullPointer(1);

	if(str->ptr)
		return Error_invalidParameter(1, 0);

	const C8 *reg = instr.reg == 8 ? "(HL)" : (instr.reg > 8 ? "" : GBRegisters_reg8[instr.reg]);
	const C8 *reg1 = instr.reg1 == 8 ? "(HL)" : (instr.reg1 > 8 ? "" : GBRegisters_reg8[instr.reg1]);

	const C8 *raw = NULL;

	switch (instr.opcodeType) {

		//Single argument instructions

		case NOP:	raw = "NOP";		break;
		case STOP:	raw = "STOP 0";		break;
		case HALT:	raw = "HALT";		break;
		case EI:	raw = "EI";			break;
		case DI:	raw = "DI";			break;
		case DAA:	raw = "DAA";		break;
		case SCF:	raw = "SCF";		break;
		case CPL:	raw = "CPL";		break;
		case CCF:	raw = "CCF";		break;
		case RLCA:	raw = "RLCA";		break;
		case RLA:	raw = "RLA";		break;
		case RRCA:	raw = "RRCA";		break;
		case RRA:	raw = "RRA";		break;
		case RETI:	raw = "RETI";		break;

		//Reset 

		case RST:
			return CharString_formatx(
				str, "RST %c%cH", 
				C8_hex((U8)(instr.intermediate >>  4)),
				C8_hex((U8)(instr.intermediate & 0xF))
			);

		//Call

		case JP:
		case JR:
		case RET:
		case CALL: {

			static const C8 *conditions[] = { "Z,", "C,", "NZ,", "NC,", "" };

			const C8 *name = "JP";

			switch (instr.opcodeType) {
				case JR:	name = "JR";	break;
				case RET:	name = "RET";	break;
				case CALL:	name = "CALL";	break;
			}

			return CharString_formatx(
				str, 
				"%s %s$%X",
				name,
				conditions[instr.reg1],
				instr.opcodeType == JR ? (I8) instr.intermediate : 
				(int) instr.intermediate
			);
		}

		//Load instructions

		case LD_REG_TO_REG:
			return CharString_formatx(str, "LD %s,%s", reg, reg1);

		case LD8_TO_REG:
			return CharString_formatx(str, "LD %s,$%X", reg, instr.intermediate);

		case LD_SP:
			return CharString_formatx(str, "LD (%u),SP", instr.intermediate);

		case LD16_TO_REG:
			return CharString_formatx(
				str, "LD %s,$%X", GBRegisters_reg16[instr.reg], instr.intermediate
			);

		case LDA16:
			return CharString_formatx(str, instr.reg ? "LD A,($%04X)" : "LD ($%04X),A", instr.intermediate);

		case LDHA:

			if(!instr.reg1 || !instr.reg)
				return CharString_formatx(str, instr.reg == 1 ? "LD A,($FF00+%X)" : "LD ($FF00+%X),A", instr.intermediate);

			return CharString_formatx(str, instr.reg == 1 ? "LD A,($FF00+C)" : "LD ($FF00+C),A");

		case LD_A_FROM_ADDR:
		case LD_A_TO_ADDR:
			return CharString_formatx(
				str, 
				instr.opcodeType == LD_A_TO_ADDR ? "LD (%s%s),A" : "LD A,(%s%s)", 
				GBRegisters_reg16[instr.reg], 
				instr.intermediate == 2 ? "-" : (
					instr.intermediate == 1 ? "+" : ""
				)
			);

		//ALU instructions

		case ADD: case ADC: case SBC: {

			const C8 *name = instr.opcodeType == ADD ? "ADD" : (instr.opcodeType == ADC ? "ADC" : "SBC");

			return 
				instr.reg <= 8 ? CharString_formatx(str, "%s A,%s", name, reg) :
				CharString_formatx(str, "%s A,$%X", name, instr.intermediate);
		}

		case INC16:
		case DEC16:
		case ADD16_HL:

			return CharString_formatx(
				str, 
				instr.opcodeType == ADD16_HL ? "ADD HL,%s" : (
					instr.opcodeType == INC16 ? "INC %s" : "DEC %s"
				),
				GBRegisters_reg16[instr.reg]
			);

		case SUB: 
		case AND: case XOR: case OR:
		case CP: 
		case INC: case DEC: {

			const C8 *name = "SUB";
			
			switch(instr.opcodeType) {
				case AND:	name = "AND";	break;
				case XOR:	name = "XOR";	break; 
				case OR:	name = "OR";	break;
				case CP:	name = "CP";	break;
				case INC:	name = "INC";	break;
				case DEC:	name = "DEC";	break;
			}

			return 
				instr.reg <= 8 ? CharString_formatx(str, "%s %s", name, reg) :
				CharString_formatx(str, "%s $%X", name, instr.intermediate);
		}

		//Pop/push

		case POP: case PUSH:
			return CharString_formatx(
				str, 
				instr.opcodeType == PUSH ? "PUSH %s" : "POP %s", 
				GBRegisters_reg16[instr.reg]
			);

		//CB instructions

		//Bit manipulation

		case RLC:	return CharString_formatx(str, "RLC %s", reg);
		case RRC:	return CharString_formatx(str, "RRC %s", reg);
		case RL:	return CharString_formatx(str, "RL %s", reg);
		case RR:	return CharString_formatx(str, "RR %s", reg);
		case SLA:	return CharString_formatx(str, "SLA %s", reg);
		case SRA:	return CharString_formatx(str, "SRA %s", reg);
		case SWAP:	return CharString_formatx(str, "SWAP %s", reg);
		case SRL:	return CharString_formatx(str, "SRL %s", reg);

		//Access individual bits

		case BIT:
		case RES:
		case SET:
			return CharString_formatx(
				str, 
				instr.opcodeType == BIT ? "BIT %u,%s" : (
					instr.opcodeType == RES ? "RES %u,%s" : 
					"SET %u,%s"
				),
				(U16)instr.intermediate, 
				reg
			);

		//Undefined

		default:
			return Error_unsupportedOperation(0);
	}

	if(raw)
		*str = CharString_createConstRefCStr(raw);

	return Error_none();
}

Error GBInstruction_deserialize(CharString str, GBInstruction *instr);
