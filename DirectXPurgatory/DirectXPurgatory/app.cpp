#include "app.h"

void Application::Create()
{
	// Create a window using SDL
	SDL_Window* window = SDL_CreateWindow(
		"DirectX 12 Purgatory",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0);
	hWnd = GetActiveWindow();
}

HWND Application::GetWindow() { return hWnd; }

bool Application::IsRunning() { return running; }

SCREEN_STATE Application::GetScreenState() { return screenState; }

float Application::GetWindowWidth() { return currentWindowWidth; }

float Application::GetWindowHeight() { return currentWindowHeight; }

void Application::StopRunning() { running = false; }
