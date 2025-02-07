#include "app.h"

int main(int argc, char* args[]) {
	
	Application* mainApp = new Application();

    mainApp->Create();

	while (mainApp->IsRunning()) 
	{
		mainApp->Update();
		mainApp->Draw();
	}

	mainApp->Destroy();

	delete mainApp;
	mainApp = nullptr;



	return 0;
}
