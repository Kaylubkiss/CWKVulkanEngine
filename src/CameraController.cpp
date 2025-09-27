#include "Controller.h"
#include <SDL2/SDL.h>
#include "ApplicationGlobal.h"


enum keys {

	W = 0, A, S, D
};

static bool keys[4] = {  };

inline void ChangeCameraPosition(Camera& camera, const float& dt)
{
	if (keys[W]) { camera.MoveForward(); }
	if (keys[A]) { camera.MoveLeft(); }
	if (keys[S]) { camera.MoveBack(); }
	if (keys[D]) { camera.MoveRight(); }

	if (camera.isUpdate) 
	{
		camera.Update(dt);
	}
}

void Controller::MoveCamera(Camera& camera, const float& dt)
{
	bool updated = false;

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		const Uint8* keystates = SDL_GetKeyboardState(nullptr);

		if (e.type == SDL_QUIT)
		{
			//it should exit.
			_Application->RequestExit();
			return;
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
			}

			if (keySymbol == SDLK_ESCAPE)
			{
				if (SDL_GetRelativeMouseMode())
				{
					SDL_SetRelativeMouseMode(SDL_FALSE);
					SDL_ShowCursor(1);
				}
				else
				{
					//it should exit.
					_Application->RequestExit();
					return;
				}
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			switch (keySymbol)
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
			}
		}


		if (e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT) && e.button.state == SDL_PRESSED) 
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}

		if (e.type == SDL_MOUSEMOTION && SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			Sint32 deltaX = e.motion.xrel;
			Sint32 deltaY = e.motion.yrel;

			camera.Rotate(deltaX, deltaY);
			
		}
	}

	ChangeCameraPosition(camera, dt);

}