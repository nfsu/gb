#include "gb/emulator.hpp"
#include "system/system.hpp"
#include "system/local_file_system.hpp"
using namespace gb;

int main() {

	Buffer rom;
	
	if (oic::System::files()->read(oic::System::files()->get("./test.gb"), rom)) {
		Emulator e(rom);
		e.wait();
		return 0;
	}

	return 1;
}