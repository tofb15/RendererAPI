#pragma once

#include "Utills/Math.hpp"
#include "WindowInput.hpp"
namespace FusionReactor {

	class Texture;

	/*
		Used to store global input, dont use this directly.
		For info about this variable @See Window::GetGlobalWindowInputHandler()
	*/
	static WindowInput __GLOBAL_WINODW_INPUT_HANDLER;

	class Window {
	public:
		virtual ~Window();

		const char* GetTitle() const;
		Int2 GetDimensions() const;
		Int2 GetPosition() const;

		virtual void SetDimensions(const Int2& dimensions) = 0;
		virtual void SetDimensions(int w, int h) = 0;
		virtual void SetPosition(const Int2& position) = 0;
		virtual void SetPosition(int x, int y) = 0;
		/*
			Change the title of the window
		*/
		virtual void SetTitle(const char* title) = 0;
		/*
			This is where the window is actually created. Call once.
		*/
		virtual bool Create(int dimensionX, int dimensionY) = 0;
		/*
		   Show the window
		*/
		virtual void Show() = 0;
		/*
			Hide the window
		*/
		virtual void Hide() = 0;
		/*
			Call to handle all window events on the specific window, this Should be called every frame.
			This function will also handle the mouse and keyboard input if the current window is active.

			To get acces to the windowInput @see Window::GetWindowInputHandler();
		*/
		virtual void HandleWindowEvents() = 0;
		/*
			@return true if the window have been terminated.
		*/
		virtual bool WindowClosed() = 0;
		/*
			@return true if the window contains the mouse.
		*/
		virtual bool ContainsMouse();
		/*
			@return true if the window is in focus.
		*/
		virtual bool IsInFocus();
		/*
			Get Access to the windows input handler. This is unique to this specific window.
			Input data in this class is reseted each time HandleWindowEvents() is called and filled with the new input if the window is in focus.
			To Get Access to a Global inputhandler @See Window::GetGlobalWindowInputHandler();

			@see Window::HandleWindowEvents()

			@return an adress to the specific windows input handler
		*/
		WindowInput& GetLocalWindowInputHandler();
		/*
			Get Access to the Global input handler. This is shared by all created windows (within this application).
			To record the global input FIRST call WindowInput::Reset() on the global WindowInput class, then call HandleWindowEvents() on each window. This should be done each frame.

			@see WindowInput::Reset()
			@see Window::HandleWindowEvents()

			To get access to window specific input only, call GetLocalWindowInputHandler() insteed of this one.

			@See Window::GetLocalWindowInputHandler()

			@return an adress to the specific windows input handler
		*/
		static WindowInput& GetGlobalWindowInputHandler();

		/*
			Call this after rendering the scene but before present

			1) Render Scene
			2) Call BeginUIRendering()
			3) Use the IMGUI API to set up the UI
			4) Call EndUIRendering()
			5) Present()
		*/
		virtual void BeginUIRendering() = 0;

		virtual void* PrepareTextureForGuiRendering(Texture* texture, bool permanent = false) = 0;

		/*
			Call this after BeginUIRendering() but before present

			1) Render Scene
			2) Call BeginUIRendering()
			3) Use the IMGUI API to set up the UI
			4) Call EndUIRendering()
			5) Present()
		*/
		virtual void EndUIRendering() = 0;
	protected:
		Window();

		Int2 m_dimensions;
		Int2 m_position;
		//unsigned int handle;
		const char* m_title;

		bool m_MouseInsideWindow;
		bool m_IsInFocus;

		WindowInput m_input;
	};
}