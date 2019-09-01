#pragma once
#include "types/types.hpp"

namespace gb {

	namespace specs {

		enum Display : usz {
			width = 160,
			height = 144
		};

	}

	namespace io {

		static constexpr u16 joypad = 0xFF00;

		enum Serial : u16 {
			data = 0xFF01,
			transferControl = 0xFF02
		};

		enum Sound : u16 {

			sweep1 = 0xFF10,
			length1,
			volume1,
			freqLo1,
			freqHi1,

			length2 = 0xFF16,
			volume2,
			freqLo2,
			freqHi2,

			off3 = 0xFF1A,
			length3,
			outLvl3,
			freqLo3,
			freqHi3,

			length4 = 0xFF20,
			volume4,
			polynomialCounter4,

			channelControl,
			soundOutputTerminal,
			enableSound
		};

		enum Graphics {
			ctrl = 0xFF40,
			stat,
			scy,
			scx,
			ly,
			lyc,
			dma,
			bgp,
			obp0,
			obgp1,
			wy,
			wx
		};

		//(2 byte address << 8) | mask
		enum GraphicsFlags : u32 {
			enableLcd = 0xFF4080,
			wndTileAddr = 0xFF4040,
			enableWindow = 0xFF4020,
			bgWindowTileAddr = 0xFF4010,
			bgTileAddr = 0xFF4008,
			highSprites = 0xFF4004,			//Sprites 8x8 or 8x16
			enableSprites = 0xFF4002,
			enableBg = 0xFF4001
		};

		enum Interrupt {
			IF = 0xFF0F,	//Process an interrupt
			IE = 0xFFFF		//Interrupt enable flags
		};
	}

}