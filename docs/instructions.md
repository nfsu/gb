# Instructions

Generally the instructions are encoded with the common register in the lowest 3 bits, the additional opcode or common register in the 3 middle bits and the <u>opcode</u> in the higher 2 bits.

```cpp
union Instruction {
    
  struct {
    u8 loParam : 3;		//& 7
    u8 hiParam : 3;		//(>> 3) & 7
    u8 op : 2;			//(>> 6) & 3
  };
    
  struct {
    u8 loNibble : 4;	//& 15
    u8 shortParam : 2;	//(>> 4) & 3
  };
    
  struct {
      u8 p : 3;
      u8 neg : 1;
  }
    
  u8 i;
    
};
```

Most of the time the first struct definition is used, however for op == 0, op == 3, the opcode might be the entirety of i or stored in loParam or loNibble.

## Instruction registers

Commonly, registers are addressed through their id. Opcodes can include this id in their format. With general registers, it addresses like B, C, D, E, H, L, (HL), A. I call this a "cr" (common register).

To make this encoding match up with the registers, I swapped AF in memory to behind H, L. And I swapped F with A to make sure the common id lined up. If (HL) is used, it has to read the memory at the address mentioned in HL. This means you can't index the registers directly with the cr id, because that would give access to the F register (not directly accessible). 

For speeding up the flow of implementation; cr 8 is defined as (pc). Even though this is not a valid ASM instruction. It can only be used for reading (as ROM is not writable) and it automatically increases PC by the bytes read.

## Instructions

### OpCode 01

Represents the LD cr, cr instructions. The exception to this is the HALT instruction (LD (hl) (hl)). 

LD is equal to `getCr(hiParam) = getCr(loParam))`.

### OpCode 10

Represents generic ALU operations on the A register.

Equal to `r.a = alu<loParam>(r.a, getCr(loParam))`.

Where alu = [ ADD, ADC, SUB, SBC, AND, XOR, OR, CP ].

### OpCode 00/11

In these formats, the actual opcode is included in the lowest nibble. However, most ops are mirrors and the code is stored in the loParam.

### 00

Exceptions are:

NOP = x00; STOP = x10; LD (a16), SP = x08. 
DAA = x27, CPL = x2F, SCF = x37, CCF = x3F.

loNibble == 1: LD shortReg[shortParam], (pc)
loNibble == 9: ADD HL, shortReg[shortParam]

Generally:
```JR ```
```-```
```LD<neg> addrReg[shortParam], A```
```INC<neg> shortReg[shortparam]```
```INC getCr(hiParam)```
```DEC getCr(hiParam)```
```getCr(hiParam) = atPc<u8>()```
```opCB<c>()```