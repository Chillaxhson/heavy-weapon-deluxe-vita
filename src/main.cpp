#include <SDL2/SDL.h>
#include <vitaGL.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>
#include "Constants.h"
#include "GameEngine.h"
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

#ifdef __vita__
    // Maximize PS Vita CPU and GPU clock frequencies for stable 60 FPS
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
#endif

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Configure VitaGL OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    SDL_Window* window = SDL_CreateWindow(
        "Heavy Weapon Deluxe",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        HeavyWeapon::SCREEN_WIDTH, HeavyWeapon::SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1); // VSync enabled

    // Run game engine
    {
        HeavyWeapon::GameEngine engine;
        if (engine.Init()) {
            engine.Run();
        }
        engine.Shutdown();
    }

    // Teardown
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef __vita__
    sceKernelExitProcess(0);
#endif
    return 0;
}
