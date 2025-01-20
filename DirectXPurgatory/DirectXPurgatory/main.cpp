#include "app.h"

int main(int argc, char* args[]) {

	Application* mainApp = new Application();
	mainApp->Create();

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
			mainApp->Update();
			mainApp->Draw();
		}
	}

	mainApp->Destroy();

	delete mainApp;
	mainApp = nullptr;

	return 0;
}
