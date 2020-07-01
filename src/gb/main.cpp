#include "system/system.hpp"
#include "system/local_file_system.hpp"
#include "system/viewport_manager.hpp"
#include "gb/emulator_interface.hpp"
using namespace gb;

int main() {

	Buffer rom;
	
	if (System::files()->read("./tetris.gb", rom)) {

		Buffer bootRom;
		System::files()->read("./gb_bios.bin", bootRom);

		Graphics g;
		EmulatorInterface em(g, rom, bootRom);

		System::viewportManager()->create(
			ViewportInfo(
				"Gameboy emulator", {}, {}, 0, &em, ViewportInfo::HANDLE_INPUT
			)
		);

		while (System::viewportManager()->size())
			System::wait(250_ms);

		return 0;
	}

	return 1;
}