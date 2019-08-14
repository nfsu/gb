#pragma once
#include "registers.hpp"
#include "emu/memory.hpp"
#include "emu/stack.hpp"
#include "gb/addresses.hpp"

namespace gb {

	struct NIOHandler { 
		template<typename T, typename Memory, usz mapping> 
		static _inline_ usz exec(Memory *m, u16 addr, const T *t) { return 0; }
	};

	struct IOHandler { 
		template<typename T, typename Memory, usz mapping> 
		static _inline_ usz exec(Memory *m, u16 addr, const T *t);
	};

	struct MemoryMapper {

		static constexpr usz
			cpuStart = 0x10000,
			cpuLength = 0x10000,		//64 KiB of CPU accessible memory
			mapping = cpuStart,			//Map the cpu memory to our memory space

			ppuStart = cpuStart + cpuLength,
			ppuLength = specs::width * specs::height * 4,	//160x144 RGBA (4-byte) color buffer

			ramStart = 0x40000,
			ramLength = 0x10000,		//64 KiB of RAM banks max

			romStart = ramStart + ramLength,
			romLength = 0x200000,		//2 MiB of ROM banks max

			mmuStart = romStart + romLength,
			mmuLength = 32,				//The MMU's variables, as well as IME

			memStart = cpuStart,
			memLength = (mmuStart + mmuLength) - cpuStart;

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

		using Memory = emu::Memory16<MemoryMapper>;
		using Stack = emu::Stack<Memory, u16>;

		enum Numbers : usz {
			FLAGS = MemoryMapper::mmuStart | 31
		};

		enum Flags {
			IME = 1
		};

		Registers r;
		Memory memory;
	};

	template<typename Memory, typename T, typename IO>
	_inline_ usz MemoryMapper::map(Memory *m, u16 a, const T *t) {

		t; m;

		//TODO: Memory should also include memory not accessible to the emulator
		//		Memory banks and things like screen RAM should be mapped at 0x10000 -> n so it's not directly accessible in the program

		switch (a >> 12) {

			case 0x0: case 0x1: case 0x2: case 0x3:
			case 0x4: case 0x5: case 0x6: case 0x7:

				if constexpr (!std::is_same_v<IO, NIOHandler>)
					return IO::exec<T, Memory, mapping>(m, a, t);
				else
					return romStart + a;

			case 0xC: case 0xD:
				return ramStart + a - 0xC000;

			//Reading from 0x1E000 -> 0x1FE00 is called illegal for the GB(C) by nintendo
			//	Even though the hardware does it this way,
			//	it isn't utilized and implementing it would add an overhead which isn't worth it.

			//case 0xE:
			//	return virtualMemory[0] | (a ^ (1 << 14));

			case 0xF:

				//if(a < 0xFE00)
				//	return virtualMemory[0] | (a ^ (1 << 14));
				if constexpr(!std::is_same_v<IO, NIOHandler>)
					IO::exec<T, Memory, mapping>(m, a, t);


		}

		//Memory located in allocated range
		return mapping | a;
	}

	template<typename T, typename Memory, typename IO>
	_inline_ T MemoryMapper::read(Memory *m, u16 a) {
		usz mapped = map(m, a);
		return *(T*)mapped;
	}

	template<typename T, typename Memory, typename IO>
	_inline_ void MemoryMapper::write(Memory *m, u16 a, const T &t) {
		usz mapped = map<Memory, T, IO>(m, a, &t);
		*(T*)mapped = t;
	}

}