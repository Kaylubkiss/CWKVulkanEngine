#include "Controller.h"
#include <SDL2/SDL.h>
#include <glm/common.hpp>
#include "Application.h"

void Controller::Update() 
{
	//assert(_Application != NULL);


	//SDL_Event e;
	//while (SDL_PollEvent(&e))
	//{
	//	const Uint8* keystates = SDL_GetKeyboardState(nullptr);

	//	ImGui_ImplSDL2_ProcessEvent(&e);

	//	

	//	static glm::vec2 mousePos(window_info.width / 2, window_info.height / 2);


	//	if (e.type == SDL_QUIT)
	//	{
	//		//it should exit.
	//		_Application->RequestExit();
	//		return;
	//	}

	//	if (e.type == SDL_KEYDOWN)
	//	{
	//		switch (e.key.keysym.sym)
	//		{
	//			case SDLK_w:
	//				keys[W] = true;
	//				break;
	//			case SDLK_s:
	//				keys[S] = true;
	//				break;
	//			case SDLK_a:
	//				keys[A] = true;
	//				break;
	//			case SDLK_d:
	//				keys[D] = true;
	//				break;
	//			case (SDLK_t):
	//				_Application->ToggleObjectVisibility(e.key.keysym.sym, keystates[SDL_SCANCODE_LSHIFT]);
	//				break;
	//			case (SDLK_ESCAPE):
	//				if (SDL_GetGrabbedWindow())
	//				{
	//					SDL_SetWindowGrab(_Application->GetWindow(), SDL_FALSE);
	//					SDL_ShowCursor(1);
	//					break;
	//				}
	//				else
	//				{
	//					//it should exit.
	//					_Application->RequestExit();
	//					return;
	//				}
	//				
	//		}
	//	}
	//	
	//	
	//	if (e.type == SDL_KEYUP)
	//	{
	//		switch (e.key.keysym.sym)
	//		{
	//		case SDLK_w:
	//			keys[W] = false;
	//			break;
	//		case SDLK_s:
	//			keys[S] = false;
	//			break;
	//		case SDLK_a:
	//			keys[A] = false;
	//			break;
	//		case SDLK_d:
	//			keys[D] = false;
	//			break;
	//		}

	//	}

	//	if (e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT) && e.button.state == SDL_PRESSED)
	//	{
	//		if (_Application->WindowisFocused())
	//		{
	//			if (!_Application->guiWindowIsFocused)
	//			{
	//				/*SDL_SetRelativeMouseMode(SDL_TRUE);*/
	//			}

	//			glm::vec2 selectMouse(e.motion.x, e.motion.y);

	//		/*	std::cout << "vp: " << _Application->GetViewport().width << " " << _Application->GetViewport().height << '\n';

	//			std::cout << "mouse: " << e.motion.x << " " << e.motion.y << " " << '\n';*/

	//			_Application->SelectWorldObjects(selectMouse.x, selectMouse.y);
	//		}
	//	}

	//	Sint32 deltaX = e.motion.xrel;
	//	Sint32 deltaY = e.motion.yrel;

	//	if (!_Application->guiWindowIsFocused) 
	//	{
	//		if ((deltaX || deltaY))
	//		{
	//			if (e.type == SDL_MOUSEMOTION && SDL_GetRelativeMouseMode() == SDL_TRUE)
	//			{
	//				//this should be the center.
	//				_Application->GetCamera().Rotate(mousePos.x, mousePos.y);

	//				mousePos.x += deltaX;
	//				mousePos.y += deltaY;

	//			}
	//		}
	//	}
	//	else 
	//	{
	//		SDL_ShowCursor(1);
	//	}
	//}

	//if (keys[W])
	//{
	//	_Application->GetCamera().MoveForward();
	//}
	//
	//if (keys[S])
	//{
	//	_Application->GetCamera().MoveBack();
	//}
	//
	//if (keys[A])
	//{
	//	_Application->GetCamera().MoveLeft();
	//}
	//
	//if (keys[D])
	//{
	//	_Application->GetCamera().MoveRight();
	//}

	
}