# Gameboy & Color (DMG/CGB)

## Registers

The processor has 6 registers;

u8 A: The arithmetic register; it has a lot of ALU instructions that only work for this.
u8 F: The flag register; it is set with arithmetic operations or the POP AF instruction. 
u8 B/C/D/E/H/L: General purpose registers
u16 SP: Stack pointer (set-up by the user, points to memory, grows downwards)
u16 PC: Program counter (points to the current instruction). This is increased immediately after you read from it (by the bytes you read from it).

The BC, DE, HL registers are accessible as 16-bit registers through some instructions. AF is only accessible through POP AF and PUSH AF. 

(r) is for taking the address from the register and performing the operation on that. LD (HL), A stores A at the address pointed to by HL.

### Flag register (F)

Zero (Z) is stored in 0x80. This is set if the last operation was non-zero.
Subtract (S) is stored in 0x40. Is set if the last operation was a subtraction operation.
Half (H) is stored in 0x20. Is set if there was a carry from the lowest nibble (& 0x10).
Carry (C) is stored in 0x10. Is set if there was a carry from the highest nibble (& 0x100).

## Memory

### Read

#### ROM #0 [0,0x4000]

The first 16 KiB is ROM bank 0. The first 0x100 bytes includes the boot ROM; which provides the scrolling Nintendo logo and setting up everything, as well as interrupts and resets. At 0x100 it includes the cartridge header, where all the cartridge's information (such as title) is stored. The first instruction generally jumps to the main function of the cartridge. 

#### ROM #n [0x4000, 0x8000]

If the ROM exceeds 32 KiB, this region is swappable. With games that don't allow this, this will be bytes 0x4000->0x8000 of ROM, otherwise 0x4000 * [n, n + 1] where n > 0.

### Write



### RW

#### VRAM [0x8000, 0xA000]

#### Cartridge RAM [0xA000, 0xC000]

8 KiB RAM included on the cartridge, generally used for saving progress.

#### RAM #0 [0xC000, 0xD000]

#### RAM #n [0xD000, 0xE000]

The last 4 KiB of RAM is swappable, works the same as ROM (except writable).

#### Echo RAM [0xE000, 0xFE00]

Allows you to access 0xC000 -> 0xEE00, but should never be used in practice. This region is never used in official games.

#### OAM [0xFE00, 0xFFEA]

Object attribute memory; for drawable sprites on screen.

#### I/O Registers [0xFF00, 0xFF80]

Registers used for communicating with devices; such as the screen, sound or ppu.

#### Zero page [0xFF80, 0xFFFF]

This is very fast memory and is 127 bytes.

#### Interrupt enable flags [0xFFFF]

The last byte is the flags for enabling interrupts.



## Instruction types

See [docs/instructions.md](docs/instructions.md).

## Special thanks

### Online resources

These following resources have greatly helped me:

Part 1 - 10 [Gameboy emulation  in JavaScript](http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-The-CPU).
[The ultimate Game Boy Talk 33c3](https://www.youtube.com/watch?v=HyzD8pNlpwI).
[Gameboy opcodes diagram](http://techgate.fr/gb-doc/gameboy-opcodes.html).
[Gameboy memory controllers](http://gbdev.gg8.se/wiki/articles/Memory_Bank_Controllers).
[Gameboy CPU manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf).
Tetris assembly code.
[Gameboy memory map](http://gameboy.mongenel.com/dmg/asmmemmap.html).

Tetris assembly code.

### Gbdev discord server

The following people have helped me by giving advice (in no particular order) on how to approach certain things such as the PPU:

PinoBatch
Max (ded)
NieDzejkob
GameFuzion
garrettgu10
eightlittlebits
gekkio