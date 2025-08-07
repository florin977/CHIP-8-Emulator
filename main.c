#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <SDL3/SDL.h>

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define DISPLAY_SCALE 20

#define FONTS_SIZE 80
#define FONTS_START_ADDRESS 0x50

#define ROM_START_ADDRESS 0x200

#define OPCODE(inst) (((inst) & 0xF000) >> 12)
#define X(inst) (((inst) & 0x0F00) >> 8)
#define Y(inst) (((inst) & 0x00F0) >> 4)
#define N(inst) ((inst) & 0x000F)
#define NN(inst) ((inst) & 0x00FF)
#define NNN(inst) ((inst) & 0x0FFF)

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

typedef struct stack_t
{
    uint16_t stack[16];
    uint8_t sp;
} stack_t;

typedef struct memory_t
{
    uint16_t PC;
    uint16_t i;
    uint8_t RAM[4096];
    uint8_t v[16];
} memory_t;

typedef struct timers_t
{
    uint8_t delay_timer;
    uint8_t sound_timer;
} timers_t;

typedef struct CPU
{
    timers_t timers;
    stack_t stack;
    memory_t memory;
    uint8_t display[CHIP8_WIDTH][CHIP8_HEIGHT];
    uint8_t input;
} CPU;

uint8_t fonts[FONTS_SIZE] = {
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

FILE *open_file(const char *filename)
{
    FILE *file = NULL;
    if ((file = fopen(filename, "r+b")) == NULL)
    {
        fprintf(stderr, "Error opening the ROM\n");
        return NULL;
    }

    return file;
}

SDL_AppResult initWindow()
{
    srand(time(NULL));

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("CHIP-8 Emulator", CHIP8_WIDTH * DISPLAY_SCALE, CHIP8_HEIGHT * DISPLAY_SCALE, 0, &window, &renderer))
    {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void draw(CPU *cpu)
{
    /*
    // Black
    SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 1.0f);
    SDL_RenderClear(renderer);
    */

    // White
    SDL_SetRenderDrawColorFloat(renderer, 1.0f, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < CHIP8_WIDTH; i++)
    {
        for (int j = 0; j < CHIP8_HEIGHT; j++)
        {
            if (cpu->display[i][j])
            {
                SDL_FRect currentPixel = {i * DISPLAY_SCALE, j * DISPLAY_SCALE, DISPLAY_SCALE, DISPLAY_SCALE};

                SDL_RenderFillRect(renderer, &currentPixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void quitSDL()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Loads the font for 0-F in the RAM at 0x50-0x9F
void loadFont(CPU *cpu)
{
    for (int i = FONTS_START_ADDRESS; i < FONTS_START_ADDRESS + FONTS_SIZE; i++)
    {
        cpu->memory.RAM[i] = fonts[i - FONTS_START_ADDRESS];
    }
}

long loadROM(CPU *cpu, const char *filename)
{
    FILE *ROM = NULL;

    if ((ROM = open_file(filename)) == NULL)
    {
        exit(-1);
    }

    fseek(ROM, 0, SEEK_END);
    long ROM_size = ftell(ROM);
    fseek(ROM, 0, SEEK_SET);

    fread(cpu->memory.RAM + ROM_START_ADDRESS, sizeof(uint8_t), ROM_size, ROM);

    fclose(ROM);

    return ROM_size;
}

uint16_t fetch(CPU *cpu)
{
    uint8_t first_half = cpu->memory.RAM[cpu->memory.PC];
    uint8_t second_half = cpu->memory.RAM[cpu->memory.PC + 1];
    cpu->memory.PC += 2;

    uint16_t instruction = (first_half << 8) | second_half;

    return instruction;
}

void clearDisplay(CPU *cpu)
{
    // Black
    SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < CHIP8_WIDTH; i++)
    {
        for (int j = 0; j < CHIP8_HEIGHT; j++)
        {
            cpu->display[i][j] = 0;
        }
    }

    SDL_RenderPresent(renderer);
}

uint8_t getKey(SDL_Event *e)
{
    switch (e->key.scancode)
    {
    /* code */
    case SDL_SCANCODE_1:
    case SDL_SCANCODE_2:
    case SDL_SCANCODE_3:
    case SDL_SCANCODE_4:
        return e->key.scancode - SDL_SCANCODE_1;

    case SDL_SCANCODE_Q:
        return 4;

    case SDL_SCANCODE_W:
        return 5;

    case SDL_SCANCODE_E:
        return 6;

    case SDL_SCANCODE_R:
        return 7;

    case SDL_SCANCODE_A:
        return 8;

    case SDL_SCANCODE_S:
        return 9;

    case SDL_SCANCODE_D:
        return 10;

    case SDL_SCANCODE_F:
        return 11;

    case SDL_SCANCODE_Z:
        return 12;

    case SDL_SCANCODE_X:
        return 13;

    case SDL_SCANCODE_C:
        return 14;

    case SDL_SCANCODE_V:
        return 15;

    default:
        return 0;
    }
}

uint8_t handleEvents(SDL_Event *e, uint8_t *quit_app, CPU *cpu)
{
    while (SDL_PollEvent(&e))
        {
            if (e->type == SDL_EVENT_QUIT)
            {
                quit_app = 1;
                return 1;
            }
            if (e->type = SDL_EVENT_KEY_DOWN)
            {
                cpu->input = getKey(&e);
                return 1;
            }

            return 0;
        }
}

int main()
{
    CPU mainCPU;
    long ROM_size = 0;

    clearDisplay(&mainCPU);
    mainCPU.display[0][0] = 1;

    loadFont(&mainCPU);
    ROM_size = loadROM(&mainCPU, "ibm2.ch8");

    mainCPU.memory.PC = ROM_START_ADDRESS;

    initWindow();

    uint8_t quit_app = 0;

    while (!quit_app)
    {
        SDL_Event e;

        handleEvents(&e, &quit_app, &mainCPU);

        for (int i = 0; i < ROM_size / 2; i++)
        {
            uint16_t instruction = fetch(&mainCPU);

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

                    mainCPU.memory.v[0xF] = (sum > 0XFF) ? 1 : 0;
                    mainCPU.memory.v[x] += mainCPU.memory.v[y];
                    break;
                }

                case 5:
                {
                    mainCPU.memory.v[0xF] = (mainCPU.memory.v[x] > mainCPU.memory.v[y]) ? 1 : 0;
                    mainCPU.memory.v[x] -= mainCPU.memory.v[y];
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
                        uint8_t next_pixel_value = ((current_row & (1 << (7 - i))) >> (7 - i));
                        uint8_t current_pixel_value = mainCPU.display[mainCPU.memory.v[x] + i][mainCPU.memory.v[y] + j];

                        if (current_pixel_value ^ next_pixel_value != current_pixel_value)
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
                    while (!handleEvents(&e, &quit_app, &mainCPU))
                    {
                        // Timers should still work
                    }

                    mainCPU.memory.v[x] = mainCPU.input;
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

                }

                case 0x55:
                {

                }

                case 0x65:
                {
                    
                }
                
                }
            }

            }
        }
    }

    quitSDL();

    return 0;
}