#include <stdio.h>
#include <stdint.h>
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
    uint8_t reg[16];
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

    for (int i = 0; i < ROM_size / 2; i++)
    {
        uint16_t instruction = fetch(&mainCPU);

        uint8_t opcode = OPCODE(instruction);

        /*
        uint8_t x = X(instruction);
        uint8_t y = Y(instruction);
        uint8_t n = N(instruction);
        printf("%0X %0X %0X %0X\n", opcode, x, y, n);
        */

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
            break;
            }
                
            case 3:
            {
            break;
            }
                
            case 4:
            {
            break;
            }
                
            case 5:
            {
            break;
            }

            case 6:
            {
                uint8_t x = X(instruction);
                uint8_t nn = NN(instruction);

                mainCPU.memory.reg[x] = nn;
            break;
            }
                
            case 7:
            {
                uint8_t x = X(instruction);
                uint8_t nn = NN(instruction);

                mainCPU.memory.reg[x] += nn;
            break;
            }

            case 8:
            {
            break;
            }
                
            case 9:
            {
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
            break;
            }
                
            case 0xC:
            {
            break;
            }

            case 0xD:
            {
                uint8_t x = X(instruction);
                uint8_t y = Y(instruction);
                uint8_t n = N(instruction);

                uint16_t i_copy = mainCPU.memory.i;

                for (int j = 0; j < n; j++)
                {
                    uint8_t currentRow = mainCPU.memory.RAM[i_copy];
                    i_copy++;

                    for (int i = 0; i < 8; i++)
                    {
                        mainCPU.display[mainCPU.memory.reg[x] + i][mainCPU.memory.reg[y] + j] = ((currentRow & (1 << (7 - i))) >> (7 - i));
                    }
                }

                draw(&mainCPU);

            break;
            }
                
            case 0xE:
            {
            break;
            }
                
            case 0xF:
            {
            break;
            }
        }
    }
    
    SDL_Delay(2000);

    quitSDL();

    return 0;
}