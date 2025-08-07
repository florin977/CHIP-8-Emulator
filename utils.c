#include "utils.h"

FILE *open_file(const char *filename)
{
    FILE *file = NULL;
    if ((file = fopen(filename, "rb")) == NULL)
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
    while (SDL_PollEvent(e))
    {
        if (e->type == SDL_EVENT_QUIT)
        {
            *quit_app = 1;
            return 1;
        }
        if (e->type == SDL_EVENT_KEY_DOWN)
        {
            cpu->input = getKey(e);
            return 1;
        }

        return 0;
    }
}