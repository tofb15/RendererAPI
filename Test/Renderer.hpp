#pragma once

#include <vector>
#include "Math.hpp"

class Camera;
class Window;
class Texture;
class Mesh;
class Material;
class RenderState;
class Technique;


/*
	Contain data used to describe a object and how it should be rendered.
	This class should ONLY be used as a blueprint/prefab to create other object copying data from this class.
*/
class Blueprint {
public:
	Mesh*					mesh;
	Technique*				technique;
	std::vector<Texture*>	textures;
};

struct SubmissionItem {
	Blueprint* mesh;
	Transform transform;
};

/*
	Documentation goes here ^^
*/
class Renderer {
public:
	enum class RendererBackend
	{
		D3D11,
		D3D12,
		Vulcan,
		OpenGL
	};

	/*
		Call to create a instance of a specific renderer class used through out the game.

		@param backend directly specifies which type of renderer to return.

		@return a Renderer* to selected by the RendererBackend param
	*/
	static	Renderer*		MakeRenderer(RendererBackend backend);
	/*
		Initialize the specific renderer instance.

		@return true if the instance was initialized successfully.
	*/
	virtual bool Initialize() = 0;
	//Builder functions to create classes specific to the renderer instance created by MakeRenderer();
#pragma region Renderers Builder Functions
	virtual Camera*			MakeCamera() = 0;
	virtual Window*			MakeWindow() = 0;
	virtual Texture*		MakeTexture() = 0;
	virtual Mesh*			MakeMesh() = 0;
	virtual Material*		MakeMaterial() = 0;
	virtual RenderState*	MakeRenderState() = 0;
	virtual Technique*		MakeTechnique(Material*, RenderState*) = 0;
#pragma endregion

	/*
		Submit work that should be rendered.
	*/
	virtual void			Submit(SubmissionItem item) = 0; //How will this work with multi-threaded submissions? Should we submit an "Entity"-class insteed of a "Mesh"-class?
	/*
		Clear previusly submited work.
	*/
	virtual void			ClearSubmissions() = 0; //Should we have this?
	/*
		Render submited work to the back buffer of the passed in window. Call Present() to present the result to the screen.

		@param window, the window to render the scene to.
		@See Present()
	*/
	virtual void			Frame(Window* window) = 0; //How will this work with multi-threading? One thread to rule them all?
	/*	
		Present the last frame rendered by frame() to the window.
		@See Frame()
	*/
	virtual void			Present() = 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame() = 0; //How will this work with multi-threading?
protected:
	Renderer();
};