#include "app.h"

void Application::Create()
{
	// Create a window using SDL
	SDL_Window* window = SDL_CreateWindow(
		"DirectX 12 Purgatory", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0);
	hWnd = GetActiveWindow();

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForD3D(window);

	// Initialize Renderer & ImGui for DirectX
	renderer = new Renderer();
	if (!renderer->Init(
		hWnd,
		screenState,
		currentWindowWidth,
		currentWindowHeight))
	{
		MessageBox(0, "Failed to initialize Direct3D 12", "Error", MB_OK);
		renderer->UnInit();
		running = false;
	}
}

void Application::Destroy()
{
	renderer->WaitForPreviousFrame();
	renderer->CloseFenceEventHandle();
	renderer->UnInit();

	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	delete renderer;
	renderer = nullptr;
}

void Application::Update()
{
	SDL_Event windowEvent;
	if (SDL_PollEvent(&windowEvent))
	{
		if (windowEvent.type == SDL_EVENT_QUIT)
		{
			StopRunning();
		}

		ImGui_ImplSDL3_ProcessEvent(&windowEvent);
	}

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
