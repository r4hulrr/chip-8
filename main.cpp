#include <iostream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>

// size of memory
constexpr uint16_t mem_size = 4096;

// size of font
constexpr uint16_t font_size = 80;

// Chip 8 memory starts at address 0x200
// the previous mem space is reserved for the interpreter
constexpr uint16_t mem_start =  0x200;

// font memory starts at 0x050
constexpr uint16_t font_start = 0x050;


struct chip8
{
    std::array <uint8_t, mem_size> memory;
    uint16_t pc;
    uint16_t index;
    std::vector <uint16_t> stack;
    uint8_t delay;
    uint8_t sound;
    std::array <uint8_t, 16> reg;
    uint8_t disp_buffer[64][32];
    std::array<bool, 16> keys;
};

void initialize(chip8& Chip8);
void loop(chip8& Chip8);
void decode(chip8& Chip8, uint16_t cur_instruct);

const std::array <uint8_t, font_size> font = {
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

int main(int argc, char* argv[]){

    chip8 Chip8;
    initialize(Chip8);
    
    loop(Chip8);
    
    
}

// intializes chip 8
void initialize(chip8& Chip8){
    // set all mem, reg, disp_buffer, delay and sound to 0
    std::fill(Chip8.memory.begin(), Chip8.memory.end(), 0x00);
    std::fill(Chip8.reg.begin(),Chip8.reg.end(),0x0);
    std::fill(&Chip8.disp_buffer[0][0], &Chip8.disp_buffer[63][31], 0x0);
    std::fill(Chip8.keys.begin(),Chip8.keys.end(), 0);
    Chip8.delay = 0x0;
    Chip8.sound = 0x0;
    // store font in font address
    std::copy(font.begin(), font.end(), Chip8.memory.begin() + font_start);
    // set PC to 0x200
    Chip8.pc = mem_start;
    // set index reg to 0
    Chip8.index = 0;
    // seed to randomize
    srand(time(nullptr));
}

// loops through fetch, decode and execute
void loop(chip8& Chip8){
    // fetch instruction
    uint16_t cur_instruct = (Chip8.memory[Chip8.pc] << 8) | Chip8.memory[Chip8.pc + 1];
    Chip8.pc += 2;
    // decode instruction
    decode(Chip8, cur_instruct);
}

// decoding instructions
void decode(chip8& Chip8, uint16_t cur_instruct){
    // extract first nibble(top 4 bits)
    uint8_t first_nibble = (cur_instruct >> 12) & 0x0F; 
    // extract second, third and fourth nibble
    uint8_t x = (cur_instruct >> 8) & 0x0F;
    uint8_t y = (cur_instruct >> 4) & 0x0F;
    uint8_t n = (cur_instruct) & 0x0F;
    // form an 8 bit immediate from the third and fourth nibbles
    uint8_t nn = (y << 4) | n;
    // form a 12 bit immediate from the second, third and fourth nibbles
    uint16_t nnn = (x << 8) | (y << 4) | n;
    // get vx and vy register from the second and third nibbles
    auto& vx = Chip8.reg[x];
    auto& vy = Chip8.reg[y];
    switch(first_nibble){
        case(0x00):
            if (cur_instruct == 0x00E0) {
                std::fill(&Chip8.disp_buffer[0][0], &Chip8.disp_buffer[0][0] + 64*32, 0x0);
            }
            if (cur_instruct == 0x00EE){
                if (Chip8.stack.size() != 0){
                    Chip8.pc = Chip8.stack.back();
                    Chip8.stack.pop_back();
                }
            }
            break;
        case(0x01):
            // 1NNN
            Chip8.pc = nnn;
            break;
        case(0x02):
            // 2NNN
            Chip8.stack.push_back(Chip8.pc);
            Chip8.pc = nnn;
            break;
        case(0x03):
            // 3XNN
            if (vx == nn){
                Chip8.pc += 2;
            }
            break;
        case(0x04):
            // 4XNN
            if (vx != nn){
                Chip8.pc += 2;
            }
            break;
        case(0x05):
            // 5XY0
            if (vx == vy){
                Chip8.pc += 2;
            }
            break;
        case(0x06):
            // 6XNN
            vx = nn;
            break;
        case(0x07):
            // 7XNN
            vx += nn;
            break;
        case(0x08):
            // look at last nibble
            switch(n){
                case(0):
                    vx = vy;
                    break;
                case(1):
                    vx |= vy;
                    break;
                case(2):
                    vx &= vy;
                    break;
                case(3):
                    vx ^= vy;
                    break;
                case(4):
                    // larger temp to check for overflow
                    uint16_t sum = vx + vy;
                    vx = (uint8_t)sum;
                    // check for overflow
                    // if overflow set vf to 1
                    if (sum > 255){
                        Chip8.reg[15] = 1;
                    }else{
                        Chip8.reg[15] = 0;
                    }
                    break;
                case(5):
                    // if first operand is larger than second
                    // set vf to 1
                    if (vx > vy){
                        Chip8.reg[15] = 1;
                    } else{
                        Chip8.reg[15] = 0;
                    }
                    vx = vx - vy;
                    break;
                case(6):
                    vx = vy;
                    // set vf flag to bit shifted out
                    Chip8.reg[15] = vx & 0x01;
                    vx = vx >> 1;
                    break;
                case(7):
                    // if first operand is larger than second
                    // set vf to 1
                    if (vy > vx){
                        Chip8.reg[15] = 1;
                    } else{
                        Chip8.reg[15] = 0;
                    }
                    vx = vy - vx;
                    break;
                case(0xE):
                    vx = vy;
                    // set vf flag to bit shifted out
                    Chip8.reg[15] = vx & 0x80;
                    vx = vx << 1;
                    break;
            break;
            }
        case(0x09):
            // 9XY0
            if (vx != vy){
                Chip8.pc += 2;
            }
            break;
        case(0x0A):
            Chip8.index = nnn;
            break;
        case(0x0B):
            Chip8.pc = nnn + Chip8.reg[0];
            break;
        case(0x0C):
            // generates a random value between 0 and 254
            uint8_t random = rand() % 255;
            vx = random & nn;
            break;
        case(0x0D):
            // get x and y coordinates from vx and vy respectively
            uint8_t x_cor = vx % 64;
            uint8_t y_cor = vy % 32;
            // set vf to 0
            Chip8.reg[15] = 0;
            for (int i = 0; i < n; i++){
                uint8_t row = Chip8.memory[Chip8.index + i];
                uint8_t dest_y = (vy + i) % 32;
                for (int b=0; b < 8 ; b++){
                    bool bit = (row >> (7 - b) & 1);
                    uint8_t dest_x = (vx + b) % 64;
                    uint8_t old_buffer = Chip8.disp_buffer[dest_x][dest_y];
                    uint8_t new_buffer = old_buffer ^ bit;
                    if ( old_buffer == 1 && bit == 1){
                        // set vf = 1
                        Chip8.reg[15] = 1;
                    }
                    Chip8.disp_buffer[dest_x][dest_y] = new_buffer;
                }
            }
            break;
        case(0x0E):
            switch (n)
            {
            case(0xE):
                // EX9E - skip one instruction
                // if key corresponding to vx is pressed
                if (Chip8.keys[vx]){
                    Chip8.pc += 2;
                }
                break;
            case(0x1):
                // EXA1 - skip one instruction
                // if key corresponding to vx is not pressed
                if (!Chip8.keys[vx]){
                    Chip8.pc += 2;
                }
                break;
            default:
                break;
            }
            break;
        case(0x0F):
            switch(nn){
                case(0x07):
                    vx = Chip8.delay;
                    break;
                case(0x15):
                    Chip8.delay = vx;
                    break;
                case(0x18):
                    Chip8.sound = vx;
                    break;
                case(0x1E):
                    // larger temp to check for overflow
                    uint16_t sum_index_vx = Chip8.index + vx;
                    Chip8.index += vx;
                    // set vf to 1 if overflows
                    if (sum_index_vx >= 255){
                        Chip8.reg[15] = 1;
                    }
                    break;
                case(0x0A):
                    if(Chip8.keys[0]){
                        vx = 0x0;
                    }else if (Chip8.keys[1])
                    {
                        vx = 0x1;
                    }else if(Chip8.keys[2])
                    {
                        vx = 0x2;
                    }else if(Chip8.keys[3])
                    {
                        vx = 0x3;
                    }else if(Chip8.keys[4])
                    {
                        vx = 0x4;
                    }else if(Chip8.keys[5])
                    {
                        vx = 0x5;
                    }else if(Chip8.keys[6])
                    {
                        vx = 0x6;
                    }else if(Chip8.keys[7])
                    {
                        vx = 0x7;
                    }else if(Chip8.keys[8])
                    {
                        vx = 0x8;
                    }else if (Chip8.keys[9])
                    {
                        vx = 0x9;
                    }else if(Chip8.keys[10])
                    {
                        vx = 0xA;
                    }else if(Chip8.keys[11])
                    {
                        vx = 0xB;
                    }else if(Chip8.keys[12])
                    {
                        vx = 0xC;
                    }else if(Chip8.keys[13])
                    {
                        vx = 0xD;
                    }else if(Chip8.keys[14])
                    {
                        vx = 0xE;
                    }else if(Chip8.keys[15])
                    {
                        vx = 0xF;
                    }else{
                        Chip8.pc -= 2;
                    }
                    break;
                case(0x29):
                    Chip8.index = font_start + 5*vx;
                    break;
                case(0x33):
                    uint8_t num = vx;
                    Chip8.memory[Chip8.index + 2] = num %10;
                    num /= 10;
                    Chip8.memory[Chip8.index + 1] = num %10;
                    num /= 10;
                    Chip8.memory[Chip8.index ] = num %10;
                    break;
                case(0x55):
                    for (int i=0; i <= x; i++){
                        Chip8.memory[Chip8.index + i] = Chip8.reg[i];
                    }
                    break;
                case(0x65):
                    for (int i=0; i <= x; i++){
                        Chip8.reg[i] = Chip8.memory[Chip8.index + i]; 
                    }
                    break;
            }
            break;
        default:
            throw std::runtime_error("Invalid opcode");
    }
}