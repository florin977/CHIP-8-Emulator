#include "opcodes.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

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

CPU mainCPU;
uint8_t quit_app = 0;
SDL_Event e;
uint64_t current_timer_update;

int main()
{
    long ROM_size = 0;

    clearDisplay(&mainCPU);
    mainCPU.display[0][0] = 1;

    loadFont(&mainCPU);
    ROM_size = loadROM(&mainCPU, "keypad.ch8");

    mainCPU.memory.PC = ROM_START_ADDRESS;

    initWindow();

    mainCPU.timers.last_timer_update = SDL_GetTicks();

    while (!quit_app)
    {
        handleEvents(&e, &quit_app, &mainCPU);

        for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++)
        {
            current_timer_update = SDL_GetTicks();

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

            uint16_t instruction = fetch(&mainCPU);

            execute(instruction);
        }
    }

    quitSDL();

    return 0;
}