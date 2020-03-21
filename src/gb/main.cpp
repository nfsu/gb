#include "system/system.hpp"
#include "system/local_file_system.hpp"
#include "system/viewport_manager.hpp"
#include "gb/emulator_interface.hpp"
using namespace gb;

int main() {

	Buffer rom;
	
	if (System::files()->read("./opus5.gb", rom)) {

		Graphics g;
		EmulatorInterface em(g, rom);

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