#include "system/system.hpp"
#include "system/local_file_system.hpp"
#include "system/viewport_manager.hpp"
#include "gb/emulator.hpp"
using namespace gb;

int main() {

	Buffer rom;
	
	if (oic::System::files()->read(oic::System::files()->get("./test.gb"), rom)) {

		Emulator e(rom);

		oic::System::viewportManager()->create(
			oic::ViewportInfo("Gameboy emulator", {}, { 720, 480 }, 0, oic::ViewportInfo::NONE, &e)
		);

		e.wait();
		return 0;
	}

	return 1;
}