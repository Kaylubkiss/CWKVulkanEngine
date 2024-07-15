#include "Controller.h"
#include <SDL2/SDL.h>
#include <glm/common.hpp>
#include "Common.h"
#include "Application.h"

void Controller::Update() 
{
	assert(_Application != NULL);

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		const Uint8* keystates = SDL_GetKeyboardState(nullptr);

		ImGui_ImplSDL2_ProcessEvent(&e);

		Sint32 deltaX = e.motion.xrel;
		Sint32 deltaY = e.motion.yrel;

		static glm::vec2 mousePos(_Application->width / 2, _Application->height / 2);


		if (e.type == SDL_QUIT)
		{
			//it should exit.
			_Application->RequestExit();
			return;
		}

		if (e.type == SDL_KEYDOWN)
		{
			switch (e.key.keysym.sym)
			{
				case SDLK_w:
					keys[W] = true;
					break;
				case SDLK_s:
					keys[S] = true;
					break;
				case SDLK_a:
					keys[A] = true;
					break;
				case SDLK_d:
					keys[D] = true;
					break;
				case (SDLK_t):
					_Application->ToggleObjectVisibility(e.key.keysym.sym, keystates[SDL_SCANCODE_LSHIFT]);
					_Application->ToggleObjectVisibility(e.key.keysym.sym, keystates[SDL_SCANCODE_LSHIFT]);
					break;
				case (SDLK_ESCAPE):
					if (SDL_GetGrabbedWindow())
					{
						SDL_SetWindowGrab(_Application->GetWindow(), SDL_FALSE);
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
			}

		}
		/*else if (e.type == SDL_MOUSEMOTION)
		{
			if ((keystates[SDL_SCANCODE_LSHIFT] &&
				e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT) &&
				guiWindowIsFocused == false) || (e.button.button == SDL_BUTTON(SDL_BUTTON_MIDDLE)))
			{
				int deltaX = e.motion.xrel;
				int deltaY = e.motion.yrel;
				glm::mat4 newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { deltaX * .1f, -deltaY * .1f, 0, 1 }) * uTransform.view;
				uTransform.view = newTransform;
				memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
			}
		}*/

		if (e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT) && e.button.state == SDL_PRESSED)
		{
			if (_Application->WindowisFocused())
			{
				if (!SDL_GetGrabbedWindow())
				{
					//relativemousemode might be better
					SDL_SetWindowGrab(_Application->GetWindow(), SDL_TRUE);
					SDL_SetRelativeMouseMode(SDL_TRUE);

					if (!_Application->guiWindowIsFocused)
					{
						SDL_WarpMouseInWindow(_Application->GetWindow(), _Application->width / 2, _Application->height / 2);
						SDL_ShowCursor(0);
					}
				}
				else
				{
					if (!_Application->guiWindowIsFocused)
					{
						SDL_WarpMouseInWindow(_Application->GetWindow(), _Application->width / 2, _Application->height / 2);
						SDL_ShowCursor(0);
					}
					else
					{
						if (SDL_ShowCursor(SDL_QUERY) != 1)
						{
							SDL_ShowCursor(1);
						}
					}


					int mouseX = e.button.x;
					int mouseY = e.button.y;

					_Application->SelectWorldObjects(mouseX, mouseY);

				}
			}
		}

		if ((deltaX || deltaY) && deltaX != std::numeric_limits<int>::max() && deltaY != std::numeric_limits<int>::max())
		{
			if (e.type == SDL_MOUSEMOTION && e.button.button == SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				mousePos.x += deltaX;
				mousePos.y += deltaY;


				//this should be the center.
				_Application->GetCamera().Rotate(mousePos.x, mousePos.y);

			/*	_Application->UpdateUniformViewMatrix();*/
			}
		}
	}

	if (keys[W])
	{
		_Application->GetCamera().MoveForward();
	}
	else if (keys[S])
	{
		_Application->GetCamera().MoveBack();
	}
	else if (keys[A])
	{
		_Application->GetCamera().MoveLeft();
	}
	else if (keys[D])
	{
		_Application->GetCamera().MoveRight();
	}

	_Application->UpdateUniformViewMatrix();
}