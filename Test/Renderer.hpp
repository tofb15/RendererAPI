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
class ShaderManager;
struct ShaderProgram;


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
	Blueprint* blueprint;
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

	virtual ~Renderer();


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
	
	/*
		Create a Camera instance

		@return a pointer to a Camera instance if successful, nullptr if not
	*/
	virtual Camera*			MakeCamera() = 0;

	/*
		Create a Window instance

		@return a pointer to a Window instance if successful, nullptr if not
	*/
	virtual Window*			MakeWindow() = 0;

	/*
		Create a Texture instance

		@return a pointer to a Texture instance if successful, nullptr if not
	*/
	virtual Texture*		MakeTexture() = 0;

	/*
		Create a Mesh instance

		@return a pointer to a Mesh instance if successful, nullptr if not
	*/
	virtual Mesh*			MakeMesh() = 0;

	/*
		Create a Material instance

		@return a pointer to a Material instance if successful, nullptr if not
	*/
	virtual Material*		MakeMaterial() = 0;

	/*
		Create a RenderState instance

		@return a pointer to a RenderState instance if successful, nullptr if not
	*/
	virtual RenderState*	MakeRenderState() = 0;

	/*
		Create a Technique instance

		@return a pointer to a Technique instance if successful, nullptr if not
	*/
	virtual Technique*		MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm) = 0;

	/*
		Create a ShaderManager instance

		@return a pointer to a ShaderManager instance if successful, nullptr if not
	*/
	virtual ShaderManager*	MakeShaderManager() = 0;

	/*
		Submit work that should be rendered
		This should be called every frame for every item before calling Frame()

		@param item, an item to be rendered
		@param camera the camera which will be used for rendering
	*/
	virtual void			Submit(SubmissionItem item, Camera* camera) = 0;
	
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
	virtual void			Present(Window * window) = 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame() = 0; //How will this work with multi-threading?
protected:
	Renderer();
};