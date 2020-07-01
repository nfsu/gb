#pragma once
#include "emu/memory.hpp"
#include "emu/stack.hpp"
#include "gb/psr.hpp"
#include "gb/addresses.hpp"
#include "types/grid.hpp"

namespace gb {

	struct MemoryMapper;

	using Memory = emu::Memory16<MemoryMapper>;
	using Stack = emu::Stack<Memory, u16>;

	struct MemoryMapper {

		static constexpr usz

			cpuStart = 0x10000,
			cpuLength = 0x10000,		//64 KiB of CPU accessible memory
			mapping = cpuStart,			//Map the cpu memory to our memory space

			ramStart = 0x40000,
			ramLength = 0x10000,		//64 KiB of RAM banks max

			romStart = ramStart + ramLength,
			romLength = 0x200000,		//2 MiB of ROM banks max

			biosStart = romStart + romLength,
			biosLength = 256,

			mmuStart = biosStart + 0x10000,		//Align better
			mmuLength = 32,						//The MMU's variables, as well as IME

			memStart = cpuStart,
			memLength = (mmuStart + mmuLength) - cpuStart;

		template<typename T>
		static _inline_ T read(Memory *m, u16 a);

		static _inline_ void calculateRomOffset(Memory *m);

		template<typename T>
		static _inline_ void write(Memory *m, u16 a, const T &t);
	};

	struct Emulator {

		//Creation

		Emulator(const Buffer &rom, const Buffer &bios);
		~Emulator() = default;

		//Emulation

		void frame(const oic::Grid2D<u32> &buffer);
		void frameNoSync(const oic::Grid2D<u32> &buffer);
		void step(bool &pushScreen);

		//"Hardware" constants
		//

		enum HardwareConstants : usz {

			MBC_RAM				= (MemoryMapper::mmuStart | 0)  << 8,			//Memory bank controller memory offset
			MBC_ROM				= (MemoryMapper::mmuStart | 8)  << 8,			//Memory bank controller memory offset

			ROM_BANK			= (MemoryMapper::mmuStart | 16)  << 8,			//Memory bank controller memory offset
			RAM_BANK			= (MemoryMapper::mmuStart | 17)  << 8,			//Memory bank controller memory offset

			FLAGS				= (MemoryMapper::mmuStart    | 18) << 8,		//For switches; like enable interrupts

			IME					= FLAGS | 0x01,									//Enable interrupts
			ENABLE_ERAM			= FLAGS | 0x02,									//Enable extenral RAM (reading from it when disabled causes a crash)
			ROM_RAM_MODE_SELECT = FLAGS | 0x04,									//Selecting the upper half of ROM memory or any other RAM bank
			IS_IN_BIOS			= FLAGS | 0x08,									//Whether or not the bios is currently running

		};

		enum MemoryControllerType : u8 {

			ROM,

			MBC1,
			MBC1_R,
			MBC1_RB,

			MBC2 = 5,
			MBC2_B,

			ROM_R = 8,
			ROM_RB,

			MMM01 = 0xB,
			MMM01_R,
			MMM01_RB,

			MBC3 = 0x11,
			MBC3_R,
			MBC3_RB,

			MBC5 = 0x19,
			MBC5_R,
			MBC5_RB,
			MBC5_RUMBLE,
			MBC5_R_RUMBLE,
			MBC5_RB_RUMBLE,

			MBC6 = 0x20,

			MBC7_RB_SENSOR_RUMBLE = 0x22,

			POCKET_CAMERA = 0xFC,
			BANDAI_TAMA5 = 0xFD,
			HUC3 = 0xFE,
			HUC1_RB = 0xFF

		};

		//Memory and output

		Memory m;
		oic::Grid2D<u32> output;

		//CR mapping
		//B,C, D,E, H,L, (HL),A
		//(HL) should be handled by the instruction itself since it uses the memory model
		static constexpr usz registerMapping[] = {
			1,0, 3,2, 5,4, usz_MAX,7 
		};

		//Registers
		union {

			//8-bit registers
			struct {
				u8 c, b, e, d, l, h;
				PSR f;
				u8 a;
			};

			//8-bit registers accessed as 16-bit
			//and stack pointer & program counter
			struct {
				u16 bc, de, hl, af, sp, pc;
			};

			u8 regs[8];
			u16 lregs[6]{};
		};

		ns lastTime = 0;
		usz ppuCycle = 0;

	private:

		template<bool doSync>
		void internalFrame(const oic::Grid2D<u32> &buffer);

		//Debug

		template<bool isCb = false, typename ...args>
		_inline_ void operation(const args &...arg);

		//Setting registers

		template<u8 cr, typename T> _inline_ void set(const T &t);
		template<u8 c, typename T> _inline_ void setc(const T &t);

		//Getting value from address stored in instruction

		template<u8 cr, typename T> _inline_ T get();
		template<u8 c, typename T> _inline_ T getc();

		//Getting a short register encoded in an instruction
		template<u8 c> _inline_ u16 &shortReg();

		//Getting address encoded in instruction

		template<u8 s> _inline_ u16 addrFromReg();

		//Setting hardware flags

		template<bool enable, usz flag> _inline_ void setFlag();
		template<usz flag> _inline_ bool getFlag();
		template<usz flag> _inline_ bool getFlagFromAddress();

		template<u8 addr> _inline_ usz reset();

		//Ops

		_inline_ usz halt();
		_inline_ usz stop();

		template<u8 c> _inline_ usz ld();
		template<u8 c> _inline_ usz ldi();
		template<u8 c> _inline_ usz lds();

		template<u8 c> _inline_ usz inc();
		template<u8 c> _inline_ usz incs();

		template<u8 c> _inline_ usz performAlu(u8 b);
		template<u8 c> _inline_ usz aluOp();

		template<u8 jp, u8 check> _inline_ usz branch();
		template<u8 c, u8 code> _inline_ usz jmp();
		template<u8 c> _inline_ usz rst();

		_inline_ usz cbInstruction();

		template<u8 c> _inline_ bool cond();

		//Switch cases

		template<u8 c> _inline_ usz op256();
		template<u8 c> _inline_ usz opCb();

		//Steps

		_inline_ usz cpuStep();
		_inline_ usz interruptHandler();
		_inline_ void ppuStep(bool &pushScreen, u32 *ppu);

		//PPU helpers

		template<bool on>
		_inline_ void pushBlank(u32 *ppu, u32 *ppuEnd);

		_inline_ void pushLine(u32 *ppu);


	};

}