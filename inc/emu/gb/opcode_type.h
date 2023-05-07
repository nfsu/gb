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

//Types of instructions

typedef enum EGBOpcodeType {

	Undefined,

	NOP,				//Nothing

	LD16_TO_REG,		//Load 16-bit intermediate to register
	LD8_TO_REG,			//Load 8-bit intermediate to register

	LD_A_TO_ADDR,		//Store A into address in register (intermediate = 1: -, 2: +)
	LD_A_FROM_ADDR,		//Load A from address in register  (intermediate = 1: -, 2: +)

	LD_REG_TO_REG,		//Load register to register

	LD_SP,				//Load sp into address in register

	INC16,				//Increment 16-bit register
	INC,				//Increment 8-bit register

	DEC16,				//Decrement 16-bit register
	DEC,				//Decrement 8-bit register

	ADD,				//Add 8-bit register (or intermediate) to A
	ADC,				//Add 8-bit register (or intermediate) to A with carry
	SUB,				//Subtract 8-bit register (or intermediate)
	SBC,				//Subtract 8-bit register (or intermediate) to A with carry
	AND,				//And 8-bit register (or intermediate) with A
	XOR,				//XOR 8-bit register (or intermediate) with A
	OR,					//OR 8-bit register (or intermediate) with A
	CP,					//Compare 8-bit register (or intermediate) with A

	ADD16_HL,			//Add 16-bit register to HL
	ADD,				//Add 8-bit intermediate

	RLCA,				//Rotate left circular accumulator
	RLA,				//Rotate left accumulator
	RRCA,				//Rotate right circular accumulator
	RRA,				//Rotate right accumulator

	DAA,
	SCF,
	CPL,
	CCF,

	RETI,				//Return from interrupt

	POP,				//Pop from stack
	PUSH,				//Push to stack

	STOP,
	HALT,

	DI,					//Disable interrupts
	EI,					//Enable interrupts

	RST,				//Reset (intermediate contains address)

	RLC,				//Rotate left carry
	RRC,				//Rotate right carry
	RL,					//Rotate left
	RR,					//Rotate right
	SLA,				//Shift left accumulator
	SRA,				//Shift right accumulator
	SWAP,				//Swap A register
	SRL,				//Shift rotate left
	BIT,				//Get bit at intermediate in register
	RES,				//Reset bit at intermediate in register
	SET,				//Set bit at intermediate in register

	//reg1 = 0: zero, 1: carry, 2: non zero, 3: non carry, 4: unconditional

	JR,					//Relative jump with offset
	JP,					//Jump to address
	RET,				//Return from function
	CALL				//Call function

	//TODO:
	/*LDH (a8),A 
	LDH A,(a8)
	LD (C),A
	LD A,(C)

	ADD SP,r8 
	LD HL,SP+r8
	LD SP,HL 
	LD (a16),A 
	LD A,(a16)

	JP (HL)*/

} EGBOpcodeType;
