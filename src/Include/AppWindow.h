#pragma once
#include <SDL2/SDL.h>
#include <cstdio>

namespace AppWindow {
	static SDL_Window* Create(int win_width, int win_height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		}

		SDL_Window* window = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_width, win_height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}

		return window;
	}

	static void Exit(SDL_Window* win) 
	{
		SDL_DestroyWindow(win);

		SDL_Quit();
	}

}