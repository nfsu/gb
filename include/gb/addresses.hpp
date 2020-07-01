#pragma once
#include "types/types.hpp"

#define OVERRIDE_FPS 10

namespace gb {

	using Address = u16;

	namespace specs {

		enum Display : usz {

			width = 160,
			height = 144,

			#ifdef OVERRIDE_FPS
				refreshTimeNs = usz(1'000'000'000 / f64(OVERRIDE_FPS))
			#else
				refreshTimeNs = usz(1'000'000'000 / 59.7)
			#endif
		};

	}

	namespace io {

		static constexpr Address joypad = 0xFF00;

		enum Serial : Address {
			data = 0xFF01,
			transferControl = 0xFF02
		};

		enum Sound : Address {

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

		enum Graphics : Address {

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
			wx,

			tileSet0	= 0x8000,
			tileSet0n1	= 0x8800,
			tileSet1	= 0x9000,
			tileMap0	= 0x9800,
			tileMap1	= 0x9C00
		};

		//(Address << 8) | mask
		enum GraphicsFlags : u32 {
			enableLcd			= 0xFF4080,
			wndTileAddr			= 0xFF4040,
			enableWindow		= 0xFF4020,
			bgWindowTileAddr	= 0xFF4010,
			bgTileAddr			= 0xFF4008,
			highSprites			= 0xFF4004,			//Sprites 8x8 or 8x16
			enableSprites		= 0xFF4002,
			enableBg			= 0xFF4001
		};

		enum Interrupt : Address {
			IF = 0xFF0F,	//Process an interrupt
			IE = 0xFFFF		//Interrupt enable flags
		};
	}

}