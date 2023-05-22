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

#include "gb/mmu.h"
#include "platforms/platform.h"
#include "platforms/ext/bufferx.h"
#include "types/buffer.h"

Error GBMMU_create(GBMMU **mem) {

	if(!mem)
		return Error_nullPointer(0);

	if(*mem)
		return Error_invalidParameter(0, 0);

	Buffer buf = (Buffer) { 0 };
	Error err = Buffer_createUninitializedBytesx(sizeof(GBMMU), &buf);

	if(err.genericError)
		return err;

	*mem = (GBMMU*) buf.ptr;
	return Error_none();
}

Bool GBMMU_free(GBMMU **mem) {

	if(!mem || !*mem)
		return true;

	Buffer buf = Buffer_createManagedPtr(*mem, sizeof(**mem));
	Bool b = Buffer_freex(&buf);

	*mem = NULL;
	return b;
}

Bool GBMMU_setBios(GBMMU *mem, Buffer bios) {

	if(!mem || Buffer_length(bios) != sizeof(mem->bios))
		return false;

	mem->hasBios = true;
	Buffer_copy(Buffer_createRef(mem->bios, sizeof(mem->bios)), bios);
	return true;
}

Bool GBMMU_setRom(GBMMU *mem, Buffer rom) {

	if(!mem || Buffer_length(rom) > sizeof(mem->rom))
		return false;

	mem->hasRom = true;
	Buffer_copy(Buffer_createRef(mem->rom, sizeof(mem->rom)), rom);
	return true;
}

Bool GBMMU_setU8(GBMMU *mem, U16 addr, U8 v) {

	//First 32 KiB is ROM or bios and not accessible here!

	if(!mem || addr < 32 * KIBI)
		return false;

	if (addr < 48 * KIBI) {

		if(addr < 40 * KIBI)
			mem->VRAM[addr & 0x1FFF] = v;

		//TODO: Cartridge controls how much is available

		else mem->xRAM[addr & 0x1FFF] = v;
	}

	//(shadow) RAM

	else if (addr < 0xFDFF)
		mem->RAM[addr & 0x1FFF] = v;

	//Sprite info, I/O, zero ram (automatically indexes it)

	else mem->spriteInfo[addr - 0xFE00] = v;

	return true;
}

Bool GBMMU_setU16(GBMMU *mem, U16 addr, U16 v) {

	if(!GBMMU_setU8(mem, addr++, (U8)(v >> 8)))
		return false;

	if (!GBMMU_setU8(mem, addr++, (U8)v))		//Can't revert previous one oops
		return false;

	return true;
}

Bool GBMMU_getU8(const GBMMU *mem, U16 addr, U8 *v, U8 *cycleCounter) {

	if(!mem || !v)
		return false;

	if(cycleCounter)
		++*cycleCounter;

	//Half the address space is allocated to ROM (and bios in first section)

	if(addr < 32 * KIBI) {

		//BIOS is mapped first 256 bytes.

		if(!mem->booted && addr < sizeof(mem->bios)) {

			if(!mem->hasBios)
				return false;

			*v = *(const U8*)(mem->bios + addr);
			return true;
		}

		//ROM

		if(!mem->hasRom)
			return false;

		//TODO: Allow swapping ROM banks

		*v = *(const U8*)(mem->rom + addr);		
	}

	//VRAM, eRAM

	else if (addr < 48 * KIBI) {

		if(addr < 40 * KIBI)
			*v = mem->VRAM[addr & 0x1FFF];

		//TODO: Cartridge controls how much is available

		else *v = mem->xRAM[addr & 0x1FFF];
	}

	//(shadow) RAM

	else if (addr < 0xFDFF)
		*v = mem->RAM[addr & 0x1FFF];

	//Sprite info, I/O, zero ram (automatically indexes it)

	else *v = mem->spriteInfo[addr - 0xFE00];

	return true;
}

Bool GBMMU_getU16(const GBMMU *mem, U16 addr, U16 *v, U8 *cycleCounter) {

	if(!mem || !v)
		return false;

	U8 a, b;

	if(!GBMMU_getU8(mem, addr++, &a, cycleCounter))
		return false;

	if(!GBMMU_getU8(mem, addr++, &b, cycleCounter))
		return false;

	*v = ((U16)b << 8) | a;
	return true;
}
