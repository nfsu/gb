
namespace gb {

	//Memory emulation

	_inline_ void MemoryMapper::calculateRomOffset(Memory *m) {

		u8 bank = m->getMemory<u8>(Emulator::ROM_BANK >> 8);

		u8 mem = m->getMemory<u8>(Emulator::ROM_RAM_MODE_SELECT >> 8);
		constexpr u8 bit = Emulator::ROM_RAM_MODE_SELECT & 0xFF;

		if (mem & bit)
			bank &= 0x1F;

		m->getMemory<u64>(Emulator::MBC_ROM >> 8) = romStart | ((bank - 1) << 14);
	}

	template<typename T>
	_inline_ T MemoryMapper::read(Memory *m, u16 a) {

		u8 &mem = m->getMemory<u8>(Emulator::IS_IN_BIOS >> 8);
		constexpr u8 bit = Emulator::IS_IN_BIOS & 0xFF;

		if (mem & bit && a < MemoryMapper::biosLength)
			return *(T*)(biosStart | a);

		switch (a >> 12) {

			case 0x0: case 0x1:
			case 0x2: case 0x3:
				return *(T*)(romStart | a);

			case 0x4: case 0x5:
			case 0x6: case 0x7:
				return *(T*)(m->getMemory<u64>(Emulator::MBC_ROM >> 8) | a);

			case 0xA: case 0xB:

				if (!(m->getMemory<u8>(Emulator::ENABLE_ERAM >> 8) & (Emulator::ENABLE_ERAM & 0xFF)))
					oic::System::log()->fatal("Emulator tried to access external memory, while this was not enabled");

				return *(T*)(m->getMemory<u64>(Emulator::MBC_RAM >> 8) + a);

			default:
				return *(T*)(mapping | a);
		}

	}

	template<typename T>
	_inline_ void MemoryMapper::write(Memory *m, u16 a, const T &t) {

		//TODO: Check 0x147 to see which MBC type we need (See Emulator::MemoryControllerType

		//Mappings from write to read memory

		switch (a >> 12) {

			case 0x0: case 0x1: {
			
				bool enableRAM = ((u8)t & 0xF) == 0xA;

				u8 &mem = m->getMemory<u8>(Emulator::ENABLE_ERAM >> 8);
				constexpr u8 bit = Emulator::ENABLE_ERAM & 0xFF;

				if(enableRAM) mem |= bit;
				else mem &= ~bit;

				break;
			}

			//Select lower 5 bit rom number
			//Banks 0x20, 0x40 and 0x60 are unavailable, so 125 banks

			case 0x2: case 0x3: {
					
				//TODO: MBC2 can only set the lower 4 bits

				//Lower 5-bits need to be non-zero

				u8 &romBank = m->getMemory<u8>(Emulator::ROM_BANK >> 8);

				u8 tu5 = (u8)t & 0x1F;

				romBank &= ~0x1F;
				romBank |= tu5 ? tu5 : 1;

				calculateRomOffset(m);

				break;
			}

			//Select upper 2-bit rom number or 2-bit ram number depending on the ROM_RAM_MODE_SELECT

			case 0x4: case 0x5: {

				//TODO: MBC2 doesn't support this

				u8 mem = m->getMemory<u8>(Emulator::ROM_RAM_MODE_SELECT >> 8);
				constexpr u8 bit = Emulator::ROM_RAM_MODE_SELECT & 0xFF;

				if (mem & bit) {

					u8 &romBank = m->getMemory<u8>(Emulator::ROM_BANK >> 8);

					romBank &= ~0x60;
					romBank |= (t & 3) << 5;

					calculateRomOffset(m);
				}

				else {

					u8 &ramBank = m->getMemory<u8>(Emulator::RAM_BANK >> 8);

					ramBank &= ~3;
					ramBank |= t & 3;

					m->getMemory<u64>(Emulator::MBC_RAM >> 8) = (ramStart - 0xA000) + ((t & 3) << 13);
				}
			}

			//Selecting ROM/RAM mode
			//If ROM banking mode is selected, only RAM bank 0 can be used
			//If RAM banking mode is selected, only ROM bank 0-0x1F can be used

			case 0x6: case 0x7: {

				//TODO: MBC2 doesn't support this
					
				bool useRamBankingMode = (u8)t == 0x1;

				u8 &mem = m->getMemory<u8>(Emulator::ROM_RAM_MODE_SELECT >> 8);
				constexpr u8 bit = Emulator::ROM_RAM_MODE_SELECT &0xFF;

				if (useRamBankingMode) {

					//Can use full range of ROM, but only the start of RAM

					mem |= bit;
					m->getMemory<u64>(Emulator::MBC_RAM >> 8) = ramStart - 0xA000;
					calculateRomOffset(m);
				}

				else {

					//Can only use 0-0x1F from ROM during this mode, so set that and reset our ram address

					mem &= ~bit;
					calculateRomOffset(m);

					m->getMemory<u64>(Emulator::MBC_RAM >> 8) = (ramStart - 0xA000) + (m->getMemory<u8>(Emulator::RAM_BANK >> 8) << 13);
				}

				break;
			}

			//Write to external memory 

			case 0xA: case 0xB:

				if (!(m->getMemory<u8>(Emulator::ENABLE_ERAM >> 8) & (Emulator::ENABLE_ERAM & 0xFF)))
					oic::System::log()->fatal("Emulator tried to access external memory, while this was not enabled");

				*(T*)(m->getMemory<u64>(Emulator::MBC_RAM >> 8) + a) = t;
				break;

			//Writing to internal memory

			default:
				*(T*)(mapping | a) = t;
		}
	}

}