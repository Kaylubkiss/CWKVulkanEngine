#include "CameraController.h"
#include <SDL2/SDL.h>

enum keys {

	W = 0, A, S, D
};

static std::array<bool, 4> keys = {  };

inline void ChangeCameraPosition(Camera& camera, const float& dt)
{
	if (keys[W]) { camera.MoveForward(); }
	if (keys[A]) { camera.MoveLeft(); }
	if (keys[S]) { camera.MoveBack(); }
	if (keys[D]) { camera.MoveRight(); }

	camera.Update(dt);
}

void Controller::MoveCamera( Camera& camera, float dt )
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		if (e.type == SDL_WINDOWEVENT) 
		{
			std::cout << "window event\n";
			switch (e.window.event) 
			{
				case SDL_WINDOWEVENT_CLOSE:
					//it should exit.
					_Application->RequestExit();
					break;
				case SDL_WINDOWEVENT_MINIMIZED:
					_GraphicsContext->GetWindow().isMinimized = true;
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					return;
				case SDL_WINDOWEVENT_RESTORED:
					_GraphicsContext->GetWindow().isMinimized = false;
					return;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					break;	
				case SDL_WINDOWEVENT_RESIZED:
					break;
				default:
					break;
			}

			continue;
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
				_GraphicsContext->ToggleUI(true);
				if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0)
				{
					std::cerr << SDL_GetError() << std::endl;
				}
			}
			else
			{
				_Application->RequestExit();
				return;
			}
		}

		if (SDL_GetRelativeMouseMode() == SDL_FALSE)
		{
			if (ImGui::GetIO().WantCaptureMouse)
			{
				continue;
			}

			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				_GraphicsContext->ToggleUI(false);
				if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
				{
					std::cerr << SDL_GetError() << std::endl;
				}
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
	}

	ChangeCameraPosition(camera, dt);

}