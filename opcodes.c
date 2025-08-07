#include "opcodes.h"

void execute(uint16_t instruction)
{
    uint8_t opcode = OPCODE(instruction);

    switch (opcode)
    {
    case 0:
    {
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);
        uint8_t n = N(instruction);

        if (x == 0x0 && y == 0xE && n == 0x0)
        {
            clearDisplay(&mainCPU);
        }
        else if (n == 0xE)
        {
            if (mainCPU.stack.sp <= 0)
            {
                fprintf(stderr, "Stack out of bounds\n");
                exit(-1);
            }

            mainCPU.stack.sp--;
            mainCPU.memory.PC = mainCPU.stack.stack[mainCPU.stack.sp];
        }
        else
        {
            // Call instruction
        }

        break;
    }

    case 1:
    {
        uint16_t nnn = NNN(instruction);

        mainCPU.memory.PC = nnn;
        break;
    }

    case 2:
    {
        uint16_t nnn = NNN(instruction);

        mainCPU.stack.stack[mainCPU.stack.sp] = mainCPU.memory.PC;
        mainCPU.stack.sp++;

        mainCPU.memory.PC = nnn;

        break;
    }

    case 3:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        if (mainCPU.memory.v[x] == nn)
        {
            mainCPU.memory.PC += 2;
        }

        break;
    }

    case 4:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        if (mainCPU.memory.v[x] != nn)
        {
            mainCPU.memory.PC += 2;
        }

        break;
    }

    case 5:
    {
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);

        if (mainCPU.memory.v[x] == mainCPU.memory.v[y])
        {
            mainCPU.memory.PC += 2;
        }

        break;
    }

    case 6:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        mainCPU.memory.v[x] = nn;
        break;
    }

    case 7:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        mainCPU.memory.v[x] += nn;
        break;
    }

    case 8:
    {
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);
        uint8_t n = N(instruction);

        switch (n)
        {
        case 0:
        {
            mainCPU.memory.v[x] = mainCPU.memory.v[y];
            break;
        }

        case 1:
        {
            mainCPU.memory.v[x] |= mainCPU.memory.v[y];
            break;
        }

        case 2:
        {
            mainCPU.memory.v[x] &= mainCPU.memory.v[y];
            break;
        }

        case 3:
        {
            mainCPU.memory.v[x] ^= mainCPU.memory.v[y];
            break;
        }

        case 4:
        {
            uint16_t sum = mainCPU.memory.v[x] + mainCPU.memory.v[y];

            mainCPU.memory.v[x] += mainCPU.memory.v[y];
            mainCPU.memory.v[0xF] = (sum > 0XFF) ? 1 : 0;
            break;
        }

        case 5:
        {
            mainCPU.memory.v[x] -= mainCPU.memory.v[y];
            mainCPU.memory.v[0xF] = (mainCPU.memory.v[x] >= mainCPU.memory.v[y]) ? 1 : 0;
            break;
        }

        case 6:
        {
            uint8_t LSB = mainCPU.memory.v[x] & 0x01;

            mainCPU.memory.v[0xF] = LSB;
            mainCPU.memory.v[x] >>= 1;
            break;
        }

        case 7:
        {
            mainCPU.memory.v[0xF] = (mainCPU.memory.v[y] > mainCPU.memory.v[x]) ? 1 : 0;
            mainCPU.memory.v[x] = mainCPU.memory.v[y] - mainCPU.memory.v[x];
            break;
        }

        case 0xE:
        {
            uint8_t MSB = (mainCPU.memory.v[x] & 0x80) >> 7;

            mainCPU.memory.v[0xF] = MSB;
            mainCPU.memory.v[x] <<= 1;
            break;
        }
        }
        break;
    }

    case 9:
    {
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);

        if (mainCPU.memory.v[x] != mainCPU.memory.v[y])
        {
            mainCPU.memory.PC += 2;
        }

        break;
    }

    case 0xA:
    {
        uint16_t nnn = NNN(instruction);

        mainCPU.memory.i = nnn;

        break;
    }

    case 0xB:
    {
        uint16_t nnn = NNN(instruction);

        mainCPU.memory.PC = mainCPU.memory.v[0] + nnn;

        break;
    }

    case 0xC:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        mainCPU.memory.v[x] = (rand() % 256) & nn;

        break;
    }

    case 0xD:
    {
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);
        uint8_t n = N(instruction);

        uint16_t i_copy = mainCPU.memory.i;

        mainCPU.memory.v[0xF] = 0;

        for (int j = 0; j < n; j++)
        {
            uint8_t current_row = mainCPU.memory.RAM[i_copy];
            i_copy++;

            for (int i = 0; i < 8; i++)
            {
                int x_pos = mainCPU.memory.v[x] % 64;
                int y_pos = mainCPU.memory.v[y] % 32;
                uint8_t next_pixel_value = ((current_row & (1 << (7 - i))) >> (7 - i));

                if (x_pos + i >= 64)
                {
                    continue;
                }
                if (y_pos + j >= 32)
                {
                    break;
                }

                uint8_t current_pixel_value = mainCPU.display[x_pos + i][y_pos + j];

                if (current_pixel_value == 1 && next_pixel_value == 1)
                {
                    mainCPU.memory.v[0xF] = 1;
                }

                mainCPU.display[mainCPU.memory.v[x] + i][mainCPU.memory.v[y] + j] = current_pixel_value ^ next_pixel_value;
            }
        }

        draw(&mainCPU);

        break;
    }

    case 0xE:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        switch (nn)
        {
        case 0x9E:
        {
            uint8_t reg_key = mainCPU.memory.v[x] & 0x0F;

            if (mainCPU.input == reg_key)
            {
                mainCPU.memory.PC += 2;
            }

            break;
        }

        case 0xA1:
        {
            uint8_t reg_key = mainCPU.memory.v[x] & 0x0F;

            if (mainCPU.input != reg_key)
            {
                mainCPU.memory.PC += 2;
            }

            break;
        }
        }
        break;
    }

    case 0xF:
    {
        uint8_t x = X(instruction);
        uint8_t nn = NN(instruction);

        switch (nn)
        {
        case 0x07:
        {
            mainCPU.memory.v[x] = mainCPU.timers.delay_timer;

            break;
        }

        case 0x0A:
        {
            mainCPU.memory.PC -= 2;
            
            while (!handleEvents(&e, &quit_app, &mainCPU))
            {
                if (current_timer_update - mainCPU.timers.last_timer_update > 16)
                {
                    mainCPU.timers.last_timer_update = current_timer_update;

                    if (mainCPU.timers.delay_timer > 0)
                    {
                        mainCPU.timers.delay_timer--;
                    }

                    if (mainCPU.timers.sound_timer > 0)
                    {
                        putchar('\a');
                        mainCPU.timers.sound_timer--;
                    }
                }
            }

            mainCPU.memory.v[x] = mainCPU.input;
            break;
        }

        case 0x15:
        {
            mainCPU.timers.delay_timer = mainCPU.memory.v[x];
            break;
        }

        case 0x18:
        {
            mainCPU.timers.sound_timer = mainCPU.memory.v[x];
            break;
        }

        case 0x1E:
        {
            mainCPU.memory.i += mainCPU.memory.v[x];
            break;
        }

        case 0x29:
        {
            uint8_t lowest_nibble = mainCPU.memory.v[x] & 0x0F;

            mainCPU.memory.i = FONTS_START_ADDRESS + lowest_nibble * 5;
            break;
        }

        case 0x33:
        {
            uint8_t hundreds = mainCPU.memory.v[x] / 100;
            uint8_t tens = (mainCPU.memory.v[x] / 10) % 10;
            ;
            uint8_t ones = mainCPU.memory.v[x] % 10;

            mainCPU.memory.RAM[mainCPU.memory.i] = hundreds;
            mainCPU.memory.RAM[mainCPU.memory.i + 1] = tens;
            mainCPU.memory.RAM[mainCPU.memory.i + 2] = ones;
            break;
        }

        case 0x55:
        {
            uint16_t i_copy = mainCPU.memory.i;

            for (int i = 0; i <= x; i++)
            {
                mainCPU.memory.RAM[i_copy] = mainCPU.memory.v[i];
                i_copy++;
            }
            break;
        }

        case 0x65:
        {
            uint16_t i_copy = mainCPU.memory.i;

            for (int i = 0; i <= x; i++)
            {
                mainCPU.memory.v[i] = mainCPU.memory.RAM[i_copy];
                i_copy++;
            }
            break;
        }
        }
    }
    }
}