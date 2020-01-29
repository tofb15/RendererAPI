#pragma once

#include "Math.hpp"
#include "Renderer.hpp"
#include <vector>

class Camera;
class Window;
class Texture;
class Mesh;
class Terrain;
class Material;
class RenderState;
class Technique;
class ShaderManager;
struct ShaderProgram;

/*
	Documentation goes here ^^
*/
class RenderAPI {
public:
	enum class RenderBackendAPI
	{
		D3D11,
		D3D12,
		Vulcan,
		OpenGL
	};

	enum class RendererType
	{
		Forward = 0,
		Deferred,
		Raytracing
	};

	virtual ~RenderAPI();

	/*
		Call to create a instance of a specific renderer class used through out the game.

		@param backend directly specifies which type of renderer to return.

		@return a Renderer* to selected by the RendererBackend param
	*/
	static	RenderAPI*		MakeAPI(RenderBackendAPI backend);
	
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
		Create a Terrain instance

		@return a pointer to a Terrain instance if successful, nullptr if not
	*/
	virtual Terrain*		MakeTerrain() = 0;

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
	virtual Renderer* MakeRenderer(const RendererType rendererType) = 0;
	virtual void Refresh() {};

protected:
	RenderAPI();
};