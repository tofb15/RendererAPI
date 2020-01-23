#pragma once
#include "Math.hpp"
#include <vector>

class Camera;
class Window;
class Technique;
class Mesh;
class Texture;

/*
	Contain data used to describe a object and how it should be rendered.
	This class should ONLY be used as a blueprint/prefab to create other object copying data from this class.
*/
class Blueprint {
public:
	Mesh* mesh;
	Technique* technique;
	std::vector<Texture*>	textures;
};

struct SubmissionItem {
	Blueprint* blueprint;
	Transform transform;
};

class Renderer
{
public:
	~Renderer();
	virtual bool Initialize() = 0;

	/*
		Submit work that should be rendered
		This should be called every frame for every item before calling Frame()

		@param item, an item to be rendered
		@param camera the camera which will be used for rendering
	*/
	virtual void			Submit(SubmissionItem item, Camera* camera = nullptr, unsigned char layer = 0) = 0;

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
	virtual void			Present(Window* window) = 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame() = 0; //How will this work with multi-threading?
protected:
	Renderer();

private:

};