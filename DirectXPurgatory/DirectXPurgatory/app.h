#pragma once

#include "gconst.h"

static bool running = true;

enum SCREEN_STATE {
	SS_NONE = -1,
	SS_WINDOWED = 0,
	SS_FULLSCREEN = 1
};

class Application {
public:
	void Create();
	void Destroy();

	HWND GetWindow();
	bool IsRunning();
	SCREEN_STATE GetScreenState();
	float GetWindowWidth();
	float GetWindowHeight();

	void StopRunning();

	friend class RenderSlop;

private:
	HWND hWnd;
	float currentWindowWidth = DEFAULT_WINDOW_WIDTH;
	float currentWindowHeight = DEFAULT_WINDOW_HEIGHT;
	SCREEN_STATE screenState = SS_WINDOWED;
};