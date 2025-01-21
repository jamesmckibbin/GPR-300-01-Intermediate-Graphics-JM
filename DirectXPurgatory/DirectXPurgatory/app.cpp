#include "app.h"

void Application::Create()
{
	// Create a window using SDL
	SDL_Window* window = SDL_CreateWindow(
		"DirectX 12 Purgatory",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0);
	hWnd = GetActiveWindow();

	// Initialize RenderSlop
	renderer = new RenderSlop();
	if (!renderer->Init(
		hWnd,
		screenState,
		currentWindowWidth,
		currentWindowHeight))
	{
		MessageBox(0, L"Failed to initialize Direct3D 12", L"Error", MB_OK);
		renderer->UnInit();
		running = false;
	}
}

void Application::Destroy()
{
	renderer->WaitForPreviousFrame();
	renderer->CloseFenceEventHandle();
	renderer->UnInit();

	delete renderer;
	renderer = nullptr;
}

void Application::Update()
{
	float dt = timer.GetFrameDelta();
	renderer->Update(dt);
}

void Application::Draw()
{
	renderer->Render();
}

HWND Application::GetWindow() { return hWnd; }

bool Application::IsRunning() { return running; }

SCREEN_STATE Application::GetScreenState() { return screenState; }

float Application::GetWindowWidth() { return currentWindowWidth; }

float Application::GetWindowHeight() { return currentWindowHeight; }

void Application::StopRunning() { running = false; }
