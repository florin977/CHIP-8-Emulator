#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <SDL3/SDL.h>

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define DISPLAY_SCALE 20

#define INSTRUCTIONS_PER_FRAME 15

#define FONTS_SIZE 80
#define FONTS_START_ADDRESS 0x50

#define ROM_START_ADDRESS 0x200

#define OPCODE(inst) (((inst) & 0xF000) >> 12)
#define X(inst) (((inst) & 0x0F00) >> 8)
#define Y(inst) (((inst) & 0x00F0) >> 4)
#define N(inst) ((inst) & 0x000F)
#define NN(inst) ((inst) & 0x00FF)
#define NNN(inst) ((inst) & 0x0FFF)

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
    uint64_t last_timer_update;
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

extern uint8_t fonts[FONTS_SIZE];

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern CPU mainCPU;
extern uint8_t quit_app;
extern SDL_Event e;
extern uint64_t current_timer_update;

FILE *open_file(const char *filename);
SDL_AppResult initWindow();
void draw(CPU *cpu);
void quitSDL();
void loadFont(CPU *cpu);
long loadROM(CPU *cpu, const char *filename);
uint16_t fetch(CPU *cpu);
void clearDisplay(CPU *cpu);
uint8_t getKey(SDL_Event *e);
uint8_t handleEvents(SDL_Event *e, uint8_t *quit_app, CPU *cpu);

#endif