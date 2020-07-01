#include "gb/emulator.hpp"
#include "emu/helper.hpp"
#include "system/viewport_manager.hpp"
#include "utils/timer.hpp"

#include <iostream>
#include <sstream>

#undef min

#ifndef NDEBUG
	//#define __PRINT_DEBUG__
	#define __PRINT_INSTRUCTIONS__
#endif

#ifdef __PRINT_DEBUG__

	template<typename ...args>
	static _inline_ void print(const args &...arg) {
		((std::cout << arg), ...);
	}

#else
	template<typename ...args>
	static _inline_ void print(const args &...) {}
#endif

#include "gb/memory_mapping.inc.hpp"
#include "gb/cpu.inc.hpp"
#include "gb/ppu.inc.hpp"

namespace gb {

	//Split rom buffer into separate banks
	_inline_ List<emu::ProgramMemoryRange> makeBanks(const Buffer &rom, const Buffer &bios) {

		if (rom.size() < 0x14D)
			oic::System::log()->fatal("ROM requires a minimum size of 0x14D");

		if (bios.size() && bios.size() != MemoryMapper::biosLength)
			oic::System::log()->fatal("BIOS requires to be 256 bytes");

		usz x = 0;

		for (usz i = 0x134; i < 0x14D; ++i)
			x -= usz(u8(rom[i] + 1));

		if (u8(x) != rom[0x14D])
			oic::System::log()->fatal("ROM has an invalid checksum");

		usz romBankSize = 16_KiB, romBanks = rom[0x148];

		switch (romBanks) {
			case 52: romBanks = 72;						break;
			case 53: romBanks = 80;						break;
			case 54: romBanks = 96;						break;
			default: romBanks = usz(2 << romBanks);
		}

		usz ramBankSize = 8_KiB, ramBanks = 1;

		switch (rom[0x149]) {
			case 0: ramBanks = 0;						break;
			case 1: ramBankSize = 2_KiB;				break;
			case 2: ramBanks = 1;						break;
			case 3: ramBanks = 4;						break;
			case 4: ramBanks = 16;						break;
			case 5: ramBanks = 8;						break;
			default:
				oic::System::log()->fatal("RAM banks are invalid");
		}

		using Range = emu::ProgramMemoryRange;

		auto ranges = List<Range> {
			Range { MemoryMapper::cpuStart, MemoryMapper::cpuLength, true, "CPU", "CPU Memory", {}, false },
			Range { MemoryMapper::ramStart, ramBankSize * ramBanks, true, "RAM #n", "RAM Banks", {} },
			Range { MemoryMapper::romStart, romBankSize * romBanks, false, "ROM #n", "ROM Banks", rom },
			Range { MemoryMapper::mmuStart, MemoryMapper::mmuLength, true, "MMU", "Memory Unitadditional variables", {} }
		};

		if (bios.size())
			ranges.push_back(Range { MemoryMapper::biosStart, MemoryMapper::biosLength, false, "BIOS", "BIOS Memory", bios });

		return ranges;
	}

	Emulator::Emulator(const Buffer &rom, const Buffer &bios) : m(
		makeBanks(rom, bios),
		List<Memory::Range>{
			Memory::Range { 0x0000_u16, u16(32_KiB), false, "ROM", "Readonly memory", {}, false },
			Memory::Range { 0x8000_u16, u16(8_KiB), true, "VRAM", "Video memory", {} },
			Memory::Range { 0xA000_u16, u16(8_KiB), true, "SAV", "Cartridge RAM", {}, false },
			Memory::Range { 0xC000_u16, u16(8_KiB), true, "RAM", "Random access memory", {} },
			Memory::Range { 0xE000_u16, u16(0xFE00 - 0xE000), true, "ERAM", "Echo RAM", {}, false },
			Memory::Range { 0xFE00_u16, 512_u16, true, "I/O", "Input Output registers", {} }
		})
	{
		m.getMemory<u64>(Emulator::MBC_ROM >> 8) = MemoryMapper::romStart;
		m.getMemory<u64>(Emulator::MBC_RAM >> 8) = MemoryMapper::ramStart - 0xC000;

		if(bios.size())
			setFlag<true, Emulator::IS_IN_BIOS>();
	}

	//CPU/GPU emulation

	template<bool doSync>
	void Emulator::internalFrame(const oic::Grid2D<u32> &buffer) {

		if (buffer.size()[0] == specs::height && buffer.size()[1] == specs::width)
			output = buffer;

		if (!output.linearSize())
			output = oic::Grid2D<u32>(Vec2usz(specs::height, specs::width));

		bool pushScreen{};

		pushBlank<false>(output.begin(), output.end());

		if constexpr (doSync) {

			//Ensure we're roughly at 60 fps (59.7)
			//Since this is the gameboy's refresh rate

			ns dif = oic::Timer::now() - lastTime;

			if (dif < specs::refreshTimeNs)
				oic::System::wait(specs::refreshTimeNs - dif);

		}

		#ifndef NDEBUG
			oic::System::log()->debug("Next frame");
		#endif

		while (!pushScreen) {
			ppuCycle += cpuStep();
			ppuCycle += interruptHandler();
			ppuStep(pushScreen, output.begin());
		}

		if constexpr (doSync)
			lastTime = oic::Timer::now();
	}

	void Emulator::frameNoSync(const oic::Grid2D<u32> &buffer) {
		internalFrame<false>(buffer);
	}

	void Emulator::frame(const oic::Grid2D<u32> &buffer) {
		internalFrame<true>(buffer);
	}

	void Emulator::step(bool &pushScreen) {

		if (!output.linearSize())
			output = oic::Grid2D<u32>(Vec2usz(specs::height, specs::width));

		ppuCycle += cpuStep();
		ppuCycle += interruptHandler();
		ppuStep(pushScreen, output.begin());
	}

}
