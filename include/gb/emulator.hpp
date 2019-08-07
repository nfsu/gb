#pragma once
#include "registers.hpp"
#include "emu/memory.hpp"
#include "emu/stack.hpp"
#include "types/types.hpp"

namespace gb {

	struct MemoryMapper {

		static constexpr usz
							mirrorStart = 0x1E000,
							mirrorEnd = 0x1FE00,
							mirrorBit = 1 << 14;

		//Unfortunately it requires (at least) 2, 4 or 6 additional instructions for a mirror
		//CMP eax, 0x1E000
		//BLE :eof
		//CMP eax, 0x1FE00
		//BGT :eof
		//EOR eax, 0x4000
		static __forceinline usz map(usz a) {

			if (a >= mirrorStart && a < mirrorEnd)
				return a ^ mirrorBit;

			return a;
		}

	};

	struct Emulator {

		Emulator(const Buffer &rom);
		~Emulator() = default;

		void wait();

		using Memory = emu::Memory16<0x10000, MemoryMapper>;
		using Stack = emu::Stack<Memory, u16>;

		Registers r;
		Memory memory;
	};

}