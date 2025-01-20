/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "app.h"
#include "renderslop.h"

int main(int argc, char* args[]) {

	Application* mainApp = new Application();
	mainApp->Create();

	RenderSlop* renderSlop = new RenderSlop();
	if (!renderSlop->Init(
		mainApp->GetWindow(),
		mainApp->GetScreenState(),
		mainApp->GetWindowWidth(),
		mainApp->GetWindowHeight()))
	{
		MessageBox(0, L"Failed to initialize Direct3D 12", L"Error", MB_OK);
		renderSlop->UnInit();
		return 1;
	}

	while (mainApp->IsRunning()) 
	{
		SDL_Event windowEvent;
		if (SDL_PollEvent(&windowEvent)) 
		{
			if (windowEvent.type == SDL_QUIT) 
			{
				mainApp->StopRunning();
			}
		}
		else 
		{
			renderSlop->Update();
			renderSlop->Render();
		}
	}

	renderSlop->WaitForPreviousFrame();
	renderSlop->CloseFenceEventHandle();
	renderSlop->UnInit();

	delete renderSlop;
	renderSlop = nullptr;
	delete mainApp;
	mainApp = nullptr;

	return 0;
}
