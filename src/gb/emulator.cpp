#include "gb/emulator.hpp"
#include "emu/helper.hpp"

namespace gb {

	Emulator::Emulator(const Buffer &rom):
		memory { {
			Memory::Range{ 0_u16, u16(32_KiB), false, "ROM", "Readonly memory", rom },
			Memory::Range{ 0x8000_u16, u16(8_KiB), true, "VRAM", "Video memory", {} },
			Memory::Range{ 0xA000_u16, u16(8_KiB), true, "RAM", "Random access memory", {} },
			Memory::Range{ 0xFE00_u16, 512_u16, true, "I/O", "RAM redirect", {} }
		} } {}

	_inline_ usz step(Registers &r, Emulator::Memory &mem);

	void Emulator::wait() {

		usz cycles{};

		while (true)
			cycles += step(r, memory);
	}

	//LD r, r Instruction

	template<u8 c> static _inline_ void ld(Registers &r) {
		r.regs[(c >> 3) & 7] = r.regs[c & 7];
	}
	
	#define ldCase(x) case x: ld<x>(r); return 4;
	#define ldCase2(x) ldCase(x) ldCase(x + 1)
	#define ldCase4(x) ldCase2(x) ldCase2(x + 2)
	#define ldCase8(x) ldCase4(x) ldCase2(x + 4) ldCase(x + 7)
	#define ldCase16(x) ldCase8(x) ldCase8(x + 8)
	#define ldCase32(x) ldCase16(x) ldCase16(x + 16)

	//ADD/ADC A instruction

	template<u8 c> static _inline_ void add(Registers &r) {
		if constexpr ((c & 8) != 0)	//ADC
			emu::addTo(r.f, r.a, u8(r.regs[c & 7] + r.f.carry()));
		else						//ADD
			emu::addTo(r.f, r.a, r.regs[c & 7]);
	}

	#define addCase(x) case x: add<x>(r); return 4;
	#define addCase2(x) addCase(x) addCase(x + 1)
	#define addCase4(x) addCase2(x) addCase2(x + 2)
	#define addCase8(x) addCase4(x) addCase2(x + 4) addCase(x + 7)

	//Switch case

	_inline_ usz step(Registers &r, Emulator::Memory &mem) {

		u8 opCode = mem.get<u8>(r.pc);
		++r.pc;

		switch (opCode) {

			//ld r, r
			ldCase32(0x40) ldCase16(0x60) ldCase8(0x78)

			//add A, r
			addCase8(0x80) addCase8(0x88)

			//Missing:	0x00-0x40, 0x70-0x78
			//			0x90-0xFF
			//			0x46, 0x56, 0x66, 0x76
			//			0x4E, 0x5E, 0x6E, 0x7E

			//Undefined:	0xDB, 0xDD, 0xE3, 0xE4, 0xEB
			//				0xEC, 0xED, 0xF4, 0xFC, 0xFD
		}

		throw std::exception();
		return usz_MAX;
	}

}