/* nfsu/gb, a basic gameboy (color) emulator.
*  Copyright (C) 2023 NFSU / Nielsbishere (Niels Brunekreef)
*  
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*  
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*  
*  You should have received a copy of the GNU General Public License
*  along with this program. If not, see https://github.com/Oxsomi/core3/blob/main/LICENSE.
*  Be aware that GPL3 requires closed source products to be GPL3 too if released to the public.
*  To prevent this a separate license will have to be requested at contact@osomi.net for a premium;
*  This is called dual licensing.
*/

#include "platforms/platform.h"
#include "platforms/keyboard.h"
#include "platforms/log.h"
#include "platforms/file.h"
#include "platforms/ext/errorx.h"
#include "platforms/ext/stringx.h"
#include "platforms/ext/bufferx.h"
#include "gb/emulator.h"

const Bool Platform_useWorkingDirectory = false;

void Program_exit() { }

void onButton(Window *w, InputDevice *device, InputHandle handle, Bool isDown) {

	if(device->type != EInputDeviceType_Keyboard)
		return;

	if(isDown && handle == EKey_F11)
		Window_toggleFullScreen(w);
}

void onDraw(Window *w) {

	Error err = Error_none();
	_gotoIfError(clean, Window_presentCPUBuffer(w, CharString_createNull(), 1 * SECOND));

clean:
	Error_printx(err, ELogLevel_Error, ELogOptions_Default);
}

int Program_run() {

	//Vars to clean

	Error err = Error_none();
	Buffer buf = (Buffer) { 0 };
	Window *wind = NULL;

	GBEmulator *gb = NULL;

	//Grab bios

	_gotoIfError(clean, GBEmulator_create(&gb));
	_gotoIfError(clean, File_read(CharString_createConstRefCStr("gb_bios.bin"), 1 * SECOND, &buf));

	//Start up emulator

	if(!GBEmulator_setBios(gb, buf))
		_gotoIfError(clean, Error_invalidOperation(0));

	Buffer_freex(&buf);

	while (gb->registers.pc < 0x100) {

		if(!GBEmulator_step(gb, NULL))
			break;
	}

    //Make window

	WindowManager_lock(&Platform_instance.windowManager, U64_MAX);
    
	WindowCallbacks callbacks = (WindowCallbacks) { 0 };
	callbacks.onDraw = onDraw;
	//callbacks.onUpdate = onUpdate;
	callbacks.onDeviceButton = onButton;
    
	_gotoIfError(clean, WindowManager_createPhysical(
		&Platform_instance.windowManager,
		I32x2_zero(), EResolution_get(EResolution_FHD),
		I32x2_zero(), I32x2_zero(),
		EWindowHint_ProvideCPUBuffer | EWindowHint_AllowFullscreen, 
		CharString_createConstRefCStr("NFSU Gameboy Emulator"),
		callbacks,
		EWindowFormat_rgba8,
		&wind
	));
    
	//Wait for user to close the window

	WindowManager_unlock(&Platform_instance.windowManager);
	WindowManager_waitForExitAll(&Platform_instance.windowManager, U64_MAX);

	wind = NULL;

clean:

	GBEmulator_free(&gb);
	Buffer_freex(&buf);

	Error_printx(err, ELogLevel_Error, ELogOptions_Default);
    
	if(wind && Lock_lock(&wind->lock, 5 * SECOND)) {
		Window_terminate(wind);
		Lock_unlock(&wind->lock);
	}

	WindowManager_unlock(&Platform_instance.windowManager);

    return err.genericError ? 1 : 0;
}