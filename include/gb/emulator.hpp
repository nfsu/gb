#pragma once
#include "registers.hpp"
#include "emu/memory.hpp"
#include "emu/stack.hpp"
#include "types/types.hpp"

namespace gb {

	struct NIOHandler { 
		template<typename T, typename Memory, usz mapping> 
		static _inline_ void exec(Memory *m, u16 addr, const T *t) {}
	};

	struct IOHandler { 
		template<typename T, typename Memory, usz mapping> 
		static _inline_ void exec(Memory *m, u16 addr, const T *t) {
			
			t; m;

			switch (addr & 0xFF) {

				case 0xF:

					if (m->flags[0]) {

						const u8 u = u8(*t);
						const u8 f = *(const u8*)(mapping | 0xFFFF), g = u & f;

						if(g & 1)		//V-Blank
							oic::System::log()->println("V-Blank");

						if(g & 2)		//LCD STAT
							oic::System::log()->println("LCD STAT");

						if(g & 4)		//Timer
							oic::System::log()->println("Timer");

						if(g & 8)		//Serial
							oic::System::log()->println("Serial");

						if(g & 16)		//Joypad
							oic::System::log()->println("Joypad");

						m->set(addr, u8(u & ~g));

					}

					break;

			}
		
		}
	};

	struct MemoryMapper {

		//Virtual memory located at 0x10000 -> 0x20000
		static constexpr usz virtualMemory[2] = { 0x10000, 0x10000 }, flagCount = 1;

		template<typename Memory, typename T = void, typename IO = NIOHandler>
		static _inline_ usz map(Memory *m, u16 a, const T *t = nullptr);

		template<typename T, typename Memory, typename IO = NIOHandler>
		static _inline_ T read(Memory *m, u16 a);

		template<typename T, typename Memory, typename IO = IOHandler>
		static _inline_ void write(Memory *m, u16 a, const T &t);
	};

	struct Emulator {

		Emulator(const Buffer &rom);
		~Emulator() = default;

		void wait();

		using Memory = emu::Memory16<MemoryMapper, MemoryMapper::flagCount>;
		using Stack = emu::Stack<Memory, u16>;

		Registers r;
		Memory memory;
	};

	template<typename Memory, typename T, typename IO>
	_inline_ usz MemoryMapper::map(Memory *m, u16 a, const T *t) {

		t;

		switch (a >> 12) {

			//Memory::Range{ 0x4000_u16, u16(16_KiB), false, "ROM #1", "Swappable ROM bank", banks[x] }
			case 0x4:
			case 0x5:
			case 0x6:
			case 0x7:
				return m->getBankedMemory(0, 0 /* TODO: */, a - 0x4000);

			//Memory::Range{ 0xD000_u16, u16(4_KiB), true, "RAM #1", "Swappable RAM bank", memoryBanks[x] },
			case 0xD:
				return m->getBankedMemory(1, 0 /* TODO: */, a - 0xD000);

			//Reading from 0x1E000 -> 0x1FE00 is called illegal for the GB(C) by nintendo
			//	Even though the hardware does it this way, it isn't utilized and implementing it would add an overhead which isn't worth it.

			//case 0xE:
			//	return virtualMemory[0] | (a ^ (1 << 14));

			case 0xF:

				//if(a < 0xFE00)
				//	return virtualMemory[0] | (a ^ (1 << 14));
				if constexpr(!std::is_same_v<IO, NIOHandler>)
					IO::exec<T, Memory, virtualMemory[0]>(m, a, t);


		}

		//Memory located in allocated range
		return virtualMemory[0] | a;
	}

	template<typename T, typename Memory, typename IO>
	_inline_ T MemoryMapper::read(Memory *m, u16 a) {
		return *(T*)map(m, a);
	}

	template<typename T, typename Memory, typename IO>
	_inline_ void MemoryMapper::write(Memory *m, u16 a, const T &t) {
		*(T*)map<Memory, T, IO>(m, a, &t) = t;
	}

}