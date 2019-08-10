#pragma once
#include "registers.hpp"
#include "emu/memory.hpp"
#include "emu/stack.hpp"
#include "types/types.hpp"

namespace gb {

	struct MemoryMapper {

		//Virtual memory located at 0x10000 -> 0x20000
		static constexpr usz virtualMemory[2] = { 0x10000, 0x10000 };

		template<typename Memory>
		static _inline_ usz map(Memory *m, u16 a);

		template<typename T, typename Memory>
		static _inline_ T read(Memory *m, u16 a);

		template<typename T, typename Memory>
		static _inline_ void write(Memory *m, u16 a, const T &t);
	};

	struct Emulator {

		Emulator(const Buffer &rom);
		~Emulator() = default;

		void wait();

		using Memory = emu::Memory16<MemoryMapper>;
		using Stack = emu::Stack<Memory, u16>;

		Registers r;
		Memory memory;
	};

	template<typename Memory>
	_inline_ usz MemoryMapper::map(Memory *m, u16 a) {

		switch (a >> 12) {

			//Memory::Range{ 0x4000_u16, u16(16_KiB), false, "ROM #1", "Swappable ROM bank", banks[x] }
			case 0x4:
			case 0x5:
			case 0x6:
			case 0x7:
				return m->getBankedMemory(0, 1 /* TODO: */, a - 0x4000);

			//Memory::Range{ 0xD000_u16, u16(4_KiB), true, "RAM #1", "Swappable RAM bank", memoryBanks[x] },
			case 0xD:
				return m->getBankedMemory(1, 1 /* TODO: */, a - 0xD000);

			//Reading from 0x1E000 -> 0x1FE00 is called illegal for the GB(C) by nintendo
			//	Even though the hardware does it this way, it isn't utilized and implementing it would add an overhead which isn't worth it.

			//case 0xE:
			//	return virtualMemory[0] | (a ^ (1 << 14));

			//case 0xF:
			//	if(a < 0xFE00)
			//		return virtualMemory[0] | (a ^ (1 << 14));
		}

		//Memory located in allocated range
		return virtualMemory[0] | a;
	}

	template<typename T, typename Memory>
	__forceinline T MemoryMapper::read(Memory *m, u16 a) {
		return *(T*)map(m, a);
	}

	template<typename T, typename Memory>
	__forceinline void MemoryMapper::write(Memory *m, u16 a, const T &t) {
		*(T*)map(m, a) = t;
	}

}