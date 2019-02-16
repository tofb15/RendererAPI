#pragma once

#include "Math.hpp"

class Window {
public:
	virtual void SetDimensions(Int2 dimensions) = 0;
	virtual void SetDimensions(int w, int h) = 0;
	virtual void SetPosition(Int2 position) = 0;
	virtual void SetPosition(int x, int y) = 0;
	virtual void SetTitle(const char* title) = 0; /*Change the title of the window*/
	virtual bool Create() = 0; /*This is where the window is actually created*/
	virtual void Show() = 0; /*Show the window*/
	virtual void Hide() = 0; /*Hide the window*/
	virtual void HandleWindowEvents() = 0; /*Should be called every frame to handle the window events.*/
private:
	Window();
	Int2 dimensions;
	Int2 position;
	//unsigned int handle;
	const char* title;

};