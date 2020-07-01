
namespace gb {

	//Palette

	constexpr u32 rgb(u8 r8, u8 g8, u8 b8) {
		return r8 | (g8 << 8) | (b8 << 16);
	}

	static constexpr u32 palette[5] = {
		rgb(136, 148, 87),	//Color 0
		rgb(84, 118, 89),	//Color 1
		rgb(59, 88, 76),	//Color 2
		rgb(34, 58, 50),	//Color 3
		rgb(66, 81, 3)		//OFF color
	};

	//Render a line of the display

	_inline_ void Emulator::pushLine(u32 *ppu) {

		if (!getFlagFromAddress<io::enableLcd>())
			return;

		//TODO: io::bgTileAddr

		u8
			ly = m[io::ly],
			scy = m[io::scy],
			y = ly + scy,
			scx = m[io::scx];

		//Draw background

		if (getFlagFromAddress<io::enableBg>()) {

			u8 tileY = y >> 3;
			u8 inTileY = y & 7;

			u16 startOffset = getFlagFromAddress<io::bgTileAddr>() ? io::tileMap1 : io::tileMap0;
			startOffset += tileY << 5;

			u32 *colors = ppu + ly * specs::width;

			for (u8 i = 0; i < specs::width; ++i) {

				u8 x = i + scx;
				u8 tileX = x >> 3;
				u8 inTileX = x & 7;
				u8 revInTileX = 7 - inTileX;

				u8 tile = m.getRef<u8>(startOffset + (tileX & 31));

				u16 tileAddr = io::tileSet0;	//Get start of tile
				tileAddr += (inTileY << 1) | (tile << 4_u16);

				u8 paletteId = (m.getRef<u8>(tileAddr) >> revInTileX) | ((m.getRef<u8>(tileAddr + 1) >> revInTileX) << 1);

				colors[i] = palette[paletteId];
			}

		}

		//m[io::ly] = u8(ly + 1);
	}

	//Push blank screen to the immediate buffer
	//Then push the immediate buffer to the screen buffer

	template<bool on>
	_inline_ void Emulator::pushBlank(u32 *it, u32 *end) {

		for (; it < end; ++it)
			*it = palette[on ? 0 : 4];

	}

	//Process the PPU

	_inline_ void Emulator::ppuStep(bool &pushScreen, u32 *ppu) {

		enum Modes {
			HBLANK = 0,			HBLANK_INTERVAL = 204 / 4,
			VBLANK = 1,			VBLANK_INTERVAL = 456 / 4,
			OAM = 2,			OAM_INTERVAL = 80 / 4,
			VRAM = 3,			VRAM_INTERVAL = 172 / 4
		};

		//Push populated frame

		u8 &lcdc = m.getRef<u8>(io::stat);
		u8 &ly = m.getRef<u8>(io::ly);
		u8 mode = lcdc & 3;

		switch (mode) {

			case HBLANK:

				if (ppuCycle >= HBLANK_INTERVAL) {

					++ly;

					//Push to screen and restart vblank

					if (ly == specs::height - 1) {

						pushScreen = true;

						m[io::IF] |= 1;		//Signal vblank
						mode = VBLANK;
					}

					//Start scanline

					else mode = OAM;

					goto end;
				}

				return;

			case VBLANK:

				if (ppuCycle >= VBLANK_INTERVAL) {

					++ly;

					if (ly > 153) {
						mode = OAM;
						ly = 0;
					}

					goto end;
				}

				return;

			case OAM:

				if (ppuCycle >= OAM_INTERVAL) {
					mode = VRAM;
					goto end;
				}

				return;

			case VRAM:

				if (ppuCycle >= VRAM_INTERVAL) {
					pushLine(ppu);
					mode = HBLANK;
					goto end;
				}

				return;

			end:

				ppuCycle = 0;
				lcdc &= ~3;
				lcdc |= mode;
		}

	}

}