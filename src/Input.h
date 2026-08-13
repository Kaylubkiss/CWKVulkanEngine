#ifndef INPUT_HPP
#define INPUT_HPP

#include "Camera.h"

enum keys {

	W = 0, A, S, D
};

inline static std::array<bool, 4> keys = {  };

inline void ChangeCameraPosition(Camera& camera, const float& dt)
{
	if (keys[W]) { camera.MoveForward(); }
	if (keys[A]) { camera.MoveLeft(); }
	if (keys[S]) { camera.MoveBack(); }
	if (keys[D]) { camera.MoveRight(); }

	camera.Update(dt);
}

inline void MoveCamera( Camera& camera, float dt )
{
	SDL_Event e;
	float scroll = 0.f;
	while (SDL_PollEvent(&e))
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		if (e.type == SDL_QUIT)
		{
			app.RequestExit(); //don't want to process any further input, so return here.
			return;
		}

		if (e.type == SDL_KEYUP)
		{
			switch (e.key.keysym.sym)
			{
				case SDLK_w:
					keys[W] = false;
					break;
				case SDLK_s:
					keys[S] = false;
					break;
				case SDLK_a:
					keys[A] = false;
					break;
				case SDLK_d:
					keys[D] = false;
					break;
				default:
					break;
			}
		}

		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
		{
			if (SDL_GetRelativeMouseMode() == SDL_TRUE)
			{
				if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0)
				{
					std::cerr << SDL_GetError() << std::endl;
				}

				UserInterface::Toggle(true);
			}
			else
			{
				app.RequestExit();
				return;
			}
		}

		if (SDL_GetRelativeMouseMode() == SDL_FALSE)
		{
			//if ImGui wants the input after checking for relative mode, then it will eat up the remaining inputs
			if (ImGui::GetIO().WantCaptureMouse)
			{
				continue;
			}

			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
				{
					std::cerr << SDL_GetError() << std::endl;
				}

				UserInterface::Toggle(false);
			}
		}

		if (e.type == SDL_MOUSEMOTION &&
			SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			Sint32 deltaX = e.motion.xrel;
			Sint32 deltaY = e.motion.yrel;

			camera.Rotate(deltaX, deltaY);
		}

		const SDL_Keycode& keySymbol = e.key.keysym.sym;
		if (e.type == SDL_KEYDOWN)
		{
			switch (keySymbol)
			{
				case SDLK_w:
					keys[W] = true;
					break;
				case SDLK_a:
					keys[A] = true;
					break;
				case SDLK_s:
					keys[S] = true;
					break;
				case SDLK_d:
					keys[D] = true;
					break;
				default:
					break;
			}
		}

		if (e.type == SDL_MOUSEWHEEL)
		{
			float dx = e.wheel.x;
			float dy = e.wheel.y;
			if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
			{
				dx *= -1;
				dy *= -1;
			}

			camera.UpdateSpeed(dy);
		}
	}

	ChangeCameraPosition(camera, dt);
}

#endif