/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SHAPED_WINDOW_X         150
#define SHAPED_WINDOW_Y         150
#define SHAPED_WINDOW_DIMENSION 640

typedef struct LoadedPicture
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_WindowShapeMode mode;
    const char *name;
} LoadedPicture;

void render(SDL_Renderer *renderer, SDL_Texture *texture)
{
    /* Clear render-target to blue. */
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 0xff);
    SDL_RenderClear(renderer);

    /* Render the texture. */
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    Uint8 num_pictures;
    LoadedPicture *pictures;
    int i, j;
    const SDL_DisplayMode *mode;
    SDL_PixelFormat *format = NULL;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Color black = { 0, 0, 0, 0xff };
    SDL_Event event;
    int should_exit = 0;
    unsigned int current_picture;
    int button_down;
    Uint32 pixelFormat = 0;
    int w, h, access = 0;

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (argc < 2) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Shape requires at least one bitmap file as argument.");
        exit(-1);
    }

    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL video.");
        exit(-2);
    }

    mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (!mode) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't get desktop display mode: %s", SDL_GetError());
        exit(-2);
    }

    num_pictures = argc - 1;
    pictures = (LoadedPicture *)SDL_malloc(sizeof(LoadedPicture) * num_pictures);
    if (pictures == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not allocate memory.");
        exit(1);
    }
    for (i = 0; i < num_pictures; i++) {
        pictures[i].surface = NULL;
    }
    for (i = 0; i < num_pictures; i++) {
        pictures[i].surface = SDL_LoadBMP(argv[i + 1]);
        pictures[i].name = argv[i + 1];
        if (pictures[i].surface == NULL) {
            for (j = 0; j < num_pictures; j++) {
                SDL_DestroySurface(pictures[j].surface);
            }
            SDL_free(pictures);
            SDL_Quit();
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not load surface from named bitmap file: %s", argv[i + 1]);
            exit(-3);
        }

        format = pictures[i].surface->format;
        if (SDL_ISPIXELFORMAT_ALPHA(format->format)) {
            pictures[i].mode.mode = ShapeModeBinarizeAlpha;
            pictures[i].mode.parameters.binarizationCutoff = 255;
        } else {
            pictures[i].mode.mode = ShapeModeColorKey;
            pictures[i].mode.parameters.colorKey = black;
        }
    }

    window = SDL_CreateShapedWindow("SDL_Shape test",
                                    SHAPED_WINDOW_X, SHAPED_WINDOW_Y,
                                    SHAPED_WINDOW_DIMENSION, SHAPED_WINDOW_DIMENSION,
                                    0);
    if (window == NULL) {
        for (i = 0; i < num_pictures; i++) {
            SDL_DestroySurface(pictures[i].surface);
        }
        SDL_free(pictures);
        SDL_Quit();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create shaped window for SDL_Shape.");
        exit(-4);
    }
    renderer = SDL_CreateRenderer(window, NULL, 0);
    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        for (i = 0; i < num_pictures; i++) {
            SDL_DestroySurface(pictures[i].surface);
        }
        SDL_free(pictures);
        SDL_Quit();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create rendering context for SDL_Shape window.");
        exit(-5);
    }

    for (i = 0; i < num_pictures; i++) {
        pictures[i].texture = NULL;
    }
    for (i = 0; i < num_pictures; i++) {
        pictures[i].texture = SDL_CreateTextureFromSurface(renderer, pictures[i].surface);
        if (pictures[i].texture == NULL) {
            for (i = 0; i < num_pictures; i++) {
                if (pictures[i].texture != NULL) {
                    SDL_DestroyTexture(pictures[i].texture);
                }
            }
            for (i = 0; i < num_pictures; i++) {
                SDL_DestroySurface(pictures[i].surface);
            }
            SDL_free(pictures);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create texture for SDL_shape.");
            exit(-6);
        }
    }

    should_exit = 0;
    current_picture = 0;
    button_down = 0;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Changing to shaped bmp: %s", pictures[current_picture].name);
    SDL_QueryTexture(pictures[current_picture].texture, &pixelFormat, &access, &w, &h);
    /* We want to set the window size in pixels */
    SDL_SetWindowSize(window, (int)SDL_ceilf(w / mode->display_scale), (int)SDL_ceilf(h / mode->display_scale));
    SDL_SetWindowShape(window, pictures[current_picture].surface, &pictures[current_picture].mode);
    while (should_exit == 0) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_KEY_DOWN) {
                button_down = 1;
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    should_exit = 1;
                    break;
                }
            }
            if (button_down && event.type == SDL_EVENT_KEY_UP) {
                button_down = 0;
                current_picture += 1;
                if (current_picture >= num_pictures) {
                    current_picture = 0;
                }
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Changing to shaped bmp: %s", pictures[current_picture].name);
                SDL_QueryTexture(pictures[current_picture].texture, &pixelFormat, &access, &w, &h);
                SDL_SetWindowSize(window, (int)SDL_ceilf(w / mode->display_scale), (int)SDL_ceilf(h / mode->display_scale));
                SDL_SetWindowShape(window, pictures[current_picture].surface, &pictures[current_picture].mode);
            }
            if (event.type == SDL_EVENT_QUIT) {
                should_exit = 1;
                break;
            }
        }
        render(renderer, pictures[current_picture].texture);
        SDL_Delay(10);
    }

    /* Free the textures. */
    for (i = 0; i < num_pictures; i++) {
        SDL_DestroyTexture(pictures[i].texture);
    }
    SDL_DestroyRenderer(renderer);
    /* Destroy the window. */
    SDL_DestroyWindow(window);
    /* Free the original surfaces backing the textures. */
    for (i = 0; i < num_pictures; i++) {
        SDL_DestroySurface(pictures[i].surface);
    }
    SDL_free(pictures);
    /* Call SDL_Quit() before quitting. */
    SDL_Quit();

    return 0;
}
