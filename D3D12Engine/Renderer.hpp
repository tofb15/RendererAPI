#pragma once
#include "Math.hpp"
#include "Light/LightSource.h"
#include <vector>
#include "ResourceManager.h"
#include <string>
class Camera;
class Window;
class Technique;
class Mesh;
class Texture;

struct ShaderDefine {
	std::wstring define;
	_Maybenull_ std::wstring value;
};

struct SubmissionItem {
	Blueprint* blueprint;
	Transform transform;
};

class GUI {
public:
	virtual void RenderGUI() = 0;
};

class Renderer
{
public:
	virtual ~Renderer();
	virtual bool Initialize() = 0;

	/*
		Submit work that should be rendered
		This should be called every frame for every item before calling Frame()

		@param item, an item to be rendered
		@param camera the camera which will be used for rendering. This parameter is optional but passing it can allow for optimizations like frustum culling of submited items.
		@param layer, decides the order items is rendered.
	*/
	virtual void			Submit(const SubmissionItem& item, Camera* camera = nullptr, unsigned char layer = 0) = 0;

	/*
		Clear previously submitted work
	*/
	virtual void			ClearSubmissions() = 0;

	/*
		Render submitted work to the back buffer of the passed window
		Call Present() to present the result to the screen
		Before calling this, Submit() has to be called for every item which should be rendered

		@param window, the window to render the scene to
		@param camera the camera which will be rendered from
		@See Present()
	*/
	virtual void			Frame(Window* window, Camera* camera) = 0; //How will this work with multi-threading? One thread to rule them all?

	/*
		Present the last frame rendered by Frame() to the passed window

		@param window the window to present the scene to
		@See Frame()
	*/
	virtual void  Present(Window* window, GUI* gui = nullptr) = 0;
	/*
		Clears the backbuffer
	*/
	virtual void  ClearFrame() = 0;
	/*
		Used to recompile raytracing shaders in runtime.

		@param defines, the defines to use during the recompile.
	*/
	virtual void  Refresh(const std::vector<ShaderDefine>* defines = nullptr) {};
	virtual void  SetLightSources(const std::vector<LightSource>& lights) = 0;
	/*
		Used to pass unique settings to the renderer.

		@param setting, name of the setting
		@param value, value of the setting
	*/
	virtual void  SetSetting(const std::string& setting, float value) {};
	/*
		Used to pass unique settings to the renderer.

		@param setting, name of the setting
		@param value, a pointer to the value of the setting
	*/
	virtual void  SetSetting(const std::string& setting, void* value) {};
	virtual float GetSetting(const std::string& setting) { return 0; };
	/*
		Save the last rendered frame to a file as an image
	*/
	virtual bool  SaveLastFrame(const std::string& file) { return false; };
#ifdef DO_TESTING
	virtual double* GetGPU_Timers(int& nValues, int& firstValue) { nValues = 0; return nullptr; };
#endif // DO_TESTING

protected:
	Renderer();

private:

};