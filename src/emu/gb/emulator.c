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

#include "platforms/ext/errorx.h"
#include "platforms/ext/stringx.h"
#include "platforms/ext/bufferx.h"
#include "platforms/log.h"
#include "gb/emulator.h"
#include "gb/instruction.h"
#include "gb/mmu.h"

Error GBEmulator_create(GBEmulator **emulator) {

	if(!emulator)
		return Error_nullPointer(0);

	if(*emulator)
		return Error_invalidParameter(0, 0);

	Error err = Error_none();
	Buffer mmu = (Buffer) { 0 };
	Buffer emu = (Buffer) { 0 };

	_gotoIfError(clean, Buffer_createEmptyBytesx(sizeof(GBMMU), &mmu));
	_gotoIfError(clean, Buffer_createEmptyBytesx(sizeof(GBEmulator), &emu));

	*emulator = (GBEmulator*) emu.ptr;
	(*emulator)->memory = (GBMMU*) mmu.ptr;

	return Error_none();

clean:
	Buffer_freex(&emu);
	Buffer_freex(&mmu);
	return err;
}

Bool GBEmulator_free(GBEmulator **emulator) {

	if(!emulator || !*emulator)
		return false;

	//Free mmu

	GBMMU *mmu = (*emulator)->memory;

	Buffer buf = Buffer_createManagedPtr(mmu, sizeof(*mmu));
	Buffer_freex(&buf);

	//Free emulator

	buf = Buffer_createManagedPtr(*emulator, sizeof(**emulator));
	Buffer_freex(&buf);

	//Clear pointer

	*emulator = NULL;
	return true;
}

Bool GBEmulator_setBios(GBEmulator *emulator, Buffer bios) {
	return emulator && GBMMU_setBios(emulator->memory, bios);
}

Bool GBEmulator_setRom(GBEmulator *emulator, Buffer bios) {
	return emulator && GBMMU_setRom(emulator->memory, bios);
}

Error GBEmulator_printInstructionAt(const GBEmulator *emulator, U16 addr) {

	GBInstruction instr = GBEmulator_decode(emulator, addr);

	if(!instr.opcodeType)
		return Error_invalidState(0);

	CharString str = CharString_createNull();
	Error err = GBInstruction_serialize(instr, &str);

	if(err.genericError)
		return err;

	Log_debugLn("%s", str.ptr);
	CharString_freex(&str);
	return Error_none();
}

Error GBEmulator_stringifyState(const GBEmulator *emulator, CharString *str) {

	if(!emulator)
		return Error_nullPointer(0);

	GBRegisters r = emulator->registers;

	return CharString_formatx(

		str,

		"\n"
		"a = %u (%02X)\n"
		"f = %u (%02X) (Z = %u, N = %u, H = %u, C = %u)\n"
		"b = %u (%02X)\n"
		"c = %u (%02X)\n"
		"d = %u (%02X)\n"
		"e = %u (%02X)\n"
		"l = %u (%02X)\n"
		"h = %u (%02X)\n"
		"\n"
		"af = %u (%04X)\n"
		"bc = %u (%04X)\n"
		"de = %u (%04X)\n"
		"hl = %u (%04X)\n"
		"sp = %u (%04X)\n"
		"pc = %u (%04X)\n"
		"\n",

		r.a, r.a,
		r.f, r.f, GBPSR_zero(r.f), GBPSR_subtract(r.f), GBPSR_carryHalf(r.f), GBPSR_carry(r.f),
		r.b, r.b,
		r.c, r.c,
		r.d, r.d,
		r.e, r.e,
		r.l, r.l,
		r.h, r.h,

		r.af, r.af,
		r.bc, r.bc,
		r.de, r.de,
		r.hl, r.hl,
		r.sp, r.sp,
		r.pc, r.pc
	);
}

Error GBEmulator_printState(const GBEmulator *emulator) {

	if(!emulator)
		return Error_nullPointer(0);

	CharString str = CharString_createNull();
	Error err = GBEmulator_stringifyState(emulator, &str);

	if(err.genericError)
		return err;

	Log_debugLn("%s", str.ptr);
	CharString_freex(&str);
	return Error_none();
}

Bool GBEmulator_step(GBEmulator *emulator, GBInstruction *instruction) {

	if(!emulator)
		return false;

	GBInstruction instr = GBEmulator_decode(emulator, emulator->registers.pc);

	if(!instr.opcodeType)
		return false;

	if(instruction)
		*instruction = instr;

	return GBEmulator_execute(emulator, instr);
}
