#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))

unsigned char Mem[4096];

unsigned char V[16];

unsigned short I;
unsigned short PC;

unsigned char DelayTimer;
unsigned char SoundTimer;

unsigned short Stack[32];
unsigned short SP;

bool StoreLoadIncreasesMemPointer = true;
bool ShiftYRegister = true;

bool RedrawFlag;
unsigned int InstructionCounter;


bool keys[9];
unsigned int _cyclesPer60Hz;

static char FONTSET[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

unsigned char getRandomNumber(int upper, int lower)
{
    return (rand() % (upper - lower + 1)) + lower;
}

void clearScreen(unsigned char **gfx)
{
    for (int x = 0; x < 64; x++)
    {
        for (int y = 0; y < 32; y++)
        {
            (*gfx)[y * 64 + x] = 0;
        }
    }
}

void Reset(unsigned char **gfx)
{
    PC = 0x200;  // Program counter starts at 0x200
    I = 0;      // Reset index register
    SP = 0;      // Reset stack pointer

    // Clear screen
    clearScreen(gfx);

    // Clear registers V0-VF
    for (int i = 0; i < sizeof(V); i++)
    {
        V[i] = 0;
    }

    // Load fontset
    for (int i = 0; i < 80; ++i)
    {
        Mem[i] = FONTSET[i];
    }

    // Reset timers
    DelayTimer = 0;
    SoundTimer = 0;

    InstructionCounter = 0;
    
    RedrawFlag = false;
}

void DecodeExecute(unsigned short opcode, unsigned char **gfx)
{
    switch (opcode & 0xF000)
    {
        case 0x0000:
            switch (opcode & 0x000F)
            {
                case 0x0000: // 0x00E0: Clears the screen      
                    clearScreen(gfx);
                    PC += 2;
                    break;

                case 0x000E: // 0x00EE: Returns from subroutine   
                    SP--;
                    PC = Stack[SP];
                    PC += 2;
                    break;

                default:
                    printf("unknown opcode: " + opcode);
                    break;
        
            }
            break;
        case 0x1000: // 1NNN: Jumps to address NNN. 
                {
                    unsigned short nnn = (unsigned short)(opcode & 0x0FFF);
                    PC = nnn;
                }
                break;

                case 0x2000: // 2NNN: Calls subroutine at NNN. 
                    {
                        unsigned short nnn = (unsigned short)(opcode & 0x0FFF);
                        Stack[SP] = PC;
                        SP++;
                        PC = nnn;
                    }
                    break;

                case 0x3000: // 3XNN: Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block) 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char nn = (unsigned char)(opcode & 0x00FF);
                        if (V[x] == nn)
                        {
                            PC += 2;
                        }
                        PC += 2;
                    }
                    break;

                case 0x4000: // 4XNN: Skips the next instruction if VX doesn't equal NN. (Usually the next instruction is a jump to skip a code block) 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char nn = (unsigned char)(opcode & 0x00FF);
                        if (V[x] != nn)
                        {
                            PC += 2;
                        }
                        PC += 2;
                    }
                    break;

                case 0x5000: // 5XY0: Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block) 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                        if (V[x] == V[y])
                        {
                            PC += 2;
                        }
                        PC += 2;
                    }
                    break;

                case 0x6000: // 6XNN: Sets VX to NN. 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char nn = (unsigned char)(opcode & 0x00FF);
                        V[x] = nn;
                        PC += 2;
                    }
                    break;

                case 0x7000: // 7XNN: Adds NN to VX. (Carry flag is not changed) 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char nn = (unsigned char)(opcode & 0x00FF);
                        V[x] += nn;
                        PC += 2;
                    }
                    break;

                case 0x8000:
                    {
                        switch (opcode & 0x000F)
                        {
                            case 0x0000: // 8XY0 : Sets VX to the value of VY. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[x] = V[y];
                                    PC += 2;
                                }
                                break;

                            case 0x0001: // 8XY1: Sets VX to VX or VY. (Bitwise OR operation) 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[x] = (unsigned char)(V[x] | V[y]);
                                    PC += 2;
                                }
                                break;

                            case 0x0002: // 8XY2: Sets VX to VX and VY. (Bitwise AND operation) 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[x] = (unsigned char)(V[x] & V[y]);
                                    PC += 2;
                                }
                                break;

                            case 0x0003: // 8XY3: Sets VX to VX xor VY. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[x] = (unsigned char)(V[x] ^ V[y]);
                                    PC += 2;
                                }
                                break;

                            case 0x0004: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    if (V[x] > 0xFF - V[y])
                                    {
                                        V[15] = 1;
                                    }
                                    else
                                    {
                                        V[15] = 0;
                                    }
                                    V[x] = (unsigned char)(V[x] + V[y]);
                                    PC += 2;
                                }
                                break;

                            case 0x0005: // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    if (V[x] > V[y])
                                    {
                                        V[15] = 0;
                                    }
                                    else
                                    {
                                        V[15] = 1;
                                    }
                                    V[x] = (unsigned char)(V[x] - V[y]);
                                    PC += 2;
                                }
                                break;

                            case 0x0006: // 8XY6: Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[15] = (unsigned char)(V[x] & 0x01);

                                    if (ShiftYRegister)
                                    {
                                        V[x] = (unsigned char)(V[y] >> 1);
                                    }
                                    else
                                    {
                                        V[x] = (unsigned char)(V[x] >> 1);
                                    }

                                    PC += 2;
                                }
                                break;

                            case 0x0007: // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    if ((V[y] - V[x]) < 0)
                                    {
                                        V[x] = (unsigned char)(254 + V[x] - V[y]);
                                        V[15] = 0;
                                    }
                                    else
                                    {
                                        V[x] = (unsigned char)(V[x] - V[y]);
                                        V[15] = 1;
                                    }
                                    PC += 2;
                                }
                                break;

                            case 0x000E: // 8XYE: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                                    V[15] = (unsigned char)((V[x] & 0x80) >> 7);
                                    
                                    if (ShiftYRegister)
                                    {
                                        V[x] = (unsigned char)(V[y] << 1);
                                    }
                                    else
                                    {
                                        V[x] = (unsigned char)(V[x] << 1);
                                    }
                                    
                                    PC += 2;
                                }
                                break;
                        }
                    }
                    break;

                case 0x9000: // 9XY0: Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block) 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                        if (V[x] != V[y])
                        {
                            PC += 2;
                        }
                        PC += 2;
                    }
                    break;

                case 0xA000: // ANNN: Sets I to the address NNN
                    {
                        unsigned short nnn = (unsigned short)(opcode & 0x0FFF);
                        I = nnn;
                        PC += 2;
                    }
                    break;

                case 0xB000: // BNNN: Jumps to the address NNN plus V0. 
                    {
                        unsigned short nnn = (unsigned short)(opcode & 0x0FFF);
                        PC = (unsigned short)(nnn + V[0]);
                    }
                    break;

                case 0xC000: // CXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN. 
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char nn = (unsigned char)(opcode & 0x00FF);
                        V[x] = (unsigned char)((getRandomNumber(0, 254)) & nn);
                        PC += 2;
                    }
                    break;

                case 0xD000: /*DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
                    Each row of 8 pixels is read as bit-coded starting from memory location I; I value doesn’t change after the execution of this instruction. 
                    As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn’t happen 
                    */
                    {
                        unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                        unsigned char y = (unsigned char)((opcode & 0x00F0) >> 4);
                        unsigned char n = (unsigned char)((opcode & 0x000F));

                        // get sprite from memory
                        unsigned char *sprite = malloc(sizeof(unsigned char) * n);
                        for (int a = 0; a < n; a++)
                        {
                            sprite[a] = Mem[I + a];
                        }

                        // VF should be 0 when no pixels are flipped set to unset
                        V[0x0F] = 0;

                        for (int yCounter = 0; yCounter < n; yCounter++)
                        {
                            for (int xCounter = 0; xCounter < 8; xCounter++)
                            {
                                int xAddress = V[x] + xCounter;
                                int yAddress = V[y] + yCounter;

                                // wrap around x and y addresses
                                xAddress = xAddress % 64;
                                yAddress = yAddress % 32;

                                unsigned char pixel = (unsigned char)(sprite[yCounter] & (0b10000000 >> xCounter));
                                pixel = (unsigned char)(pixel >> (7 - xCounter));

                                if (pixel == 1)
                                {
                                    if ((*gfx)[yAddress * 64 + xAddress] == 1)
                                    {
                                        V[0x0F] = 1;
                                    }
                                    (*gfx)[yAddress * 64 + xAddress] = (*gfx)[yAddress * 64 + xAddress] ^ pixel;
                                }
                            }
                        }

                        free(sprite);

                        RedrawFlag = true;

                        PC += 2;
                    }
                    break;

                case 0xE000:
                    {
                        switch (opcode & 0x00FF)
                        {
                            case 0x009E: // EX9E : Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to skip a code block) 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    if (keys[V[x]] == true)
                                    {
                                        PC += 2;
                                    }
                                    PC += 2;
                                }
                                break;

                            case 0x00A1: // EXA1 : Skips the next instruction if the key stored in VX isn't pressed. (Usually the next instruction is a jump to skip a code block)  
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    if (keys[V[x]] == false)
                                    {
                                        PC += 2;
                                    }
                                    else
                                    {

                                    }
                                    PC += 2;
                                }
                                break;
                        }
                    }
                    break;

                case 0xF000:
                    {
                        switch (opcode & 0x00FF)
                        {
                            case 0x0007: // FX07 : Sets VX to the value of the delay timer. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    V[x] = DelayTimer;
                                    PC += 2;
                                }
                                break;
                            case 0x000A: // FX0A : A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event) 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    for(int i=0; i<9; i++)
                                    {
                                        if (keys[i] == true)
                                        {
                                            V[x] = (unsigned char)i;
                                            PC += 2;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 0x0015: // FX15 : Sets the delay timer to VX. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    DelayTimer = V[x];
                                    PC += 2;
                                }
                                break;
                            case 0x0018: // FX18 : Sets the sound timer to VX. 
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    SoundTimer = V[x];
                                    PC += 2;
                                }
                                break;
                            case 0x001E: // FX1E : Adds VX to I. VF is set to 1 when there is a range overflow (I+VX>0xFFF), and to 0 when there isn't.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    if ((I + V[x]) > 0xFFF)
                                    {
                                        I = (unsigned short)(0xFFF - I + V[x]);
                                        V[15] = 1;
                                    }
                                    else
                                    {
                                        I = (unsigned short)(I + V[x]);
                                        V[15] = 0;
                                    }
                                    PC += 2;
                                }
                                break;
                            case 0x0029: // FX29 : Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    I = (unsigned char)(5 * V[x]);
                                    PC += 2;
                                }
                                break;
                            case 0x0033: // FX33  : 
                                /*
                                 * Stores the binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. 
                                 * (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.) 
                                 */
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    
                                    Mem[I] = (unsigned char)((V[x] / 100) % 10);
                                    Mem[I + 1] = (unsigned char)((V[x] / 10) % 10);
                                    Mem[I + 2] = (unsigned char)(V[x] % 10);

                                    PC += 2;
                                }
                                break;
                            case 0x0055: // FX55 : Stores V0 to VX (including VX) in memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    for (int i = 0; i <= x; i++)
                                    {
                                        Mem[I + i] = V[i];
                                    }

                                    if (StoreLoadIncreasesMemPointer)
                                    {
                                        I = (unsigned short)(I + x + 1);
                                    }

                                    PC += 2;
                                }
                                break;
                            case 0x0065: // FX65 : Fills V0 to VX (including VX) with values from memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
                                {
                                    unsigned char x = (unsigned char)((opcode & 0x0F00) >> 8);
                                    for (int i = 0; i <= x; i++)
                                    {
                                        V[i] = Mem[I + i];
                                    }

                                    if (StoreLoadIncreasesMemPointer)
                                    {
                                        I = (unsigned short)(I + x + 1);
                                    }

                                    PC += 2;
                                }                               
                                break;
                        }
                    }
                    break;
    }
}

void UpdateTimers()
{
    InstructionCounter++;
    if (InstructionCounter == _cyclesPer60Hz)
    {
        if (DelayTimer > 0)
            DelayTimer--;

        if (SoundTimer > 0)
        {
            if (SoundTimer == 1)
            {
                //Console.Beep();
            }
            SoundTimer--;
        }
        InstructionCounter = 0;
    }
}

bool cycle(unsigned char **gfx)
{
    // reset redraw
    RedrawFlag = false;

    // Load Opcode
    unsigned short opcode = (unsigned short)(Mem[PC] << 8 | (unsigned char)Mem[PC + 1]);

    // Decode Opcode
    // Execute Opcode
    DecodeExecute(opcode, gfx);

    // Update timers
    UpdateTimers();

    return RedrawFlag;
}

void Load(unsigned char **programCode, long length)
{
    const int PROGMEMSTART = 0x200;

    for (long i = 0; i < length; i++)
    {
        Mem[PROGMEMSTART + i] = (*programCode)[i];
    }
}