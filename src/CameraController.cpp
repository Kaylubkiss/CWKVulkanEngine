#include "Controller.h"
#include <SDL2/SDL.h>

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
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		ImGui_ImplSDL2_ProcessEvent(&e);

		if (e.type == SDL_WINDOWEVENT) 
		{
			switch (e.window.event) 
			{
				case SDL_WINDOWEVENT_CLOSE:
					//it should exit.
					_Application->RequestExit();
					return;
				case SDL_WINDOWEVENT_MINIMIZED:
					std::cout << "window is minimized\n\n";	
					_GraphicsContext->GetWindow().isMinimized = true;
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					std::cout << "window is maximized\n\n";
					break;
				case SDL_WINDOWEVENT_RESTORED:
					std::cout << "window is restored\n\n";
					if (_GraphicsContext->GetWindow().isMinimized) 
					{
						_GraphicsContext->GetWindow().isMinimized = false;
						
						_Application->ResizeWindow();
					}
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					std::cout << "window size changed\n\n";
					//_Application->ResizeWindow();
					_GraphicsContext->GetWindow().isPrepared = false;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					std::cout << "window focus gained\n\n";
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					std::cout << "window focus lost\n\n";
					break;
				case SDL_WINDOWEVENT_RESIZED:
					std::cout << "window is resized\n\n";
					_GraphicsContext->GetWindow().isPrepared = true;
					_Application->ResizeWindow();
					break;

				default:
					break;

					
			}
		}

		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse || io.WantCaptureKeyboard)
		{
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
				if (SDL_GetRelativeMouseMode() == SDL_TRUE)
				{
					if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0)
					{
						std::cerr << SDL_GetError() << std::endl;
					}
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

			if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0) 
			{
				std::cerr << SDL_GetError() << std::endl;
			}
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