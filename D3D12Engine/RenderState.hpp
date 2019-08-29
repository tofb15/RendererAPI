#pragma once
/*
	Specific rendering states goes here like Wireframe, conservative rasterization etc.
*/
class RenderState {
public:

	enum class FaceCulling
	{
		NONE = 1,
		FRONT = 2,
		BACK = 3,
	};

	virtual ~RenderState();

	// Sets the wireframe variable to true or false.
	/*
		@param wf, set this to true if objects should be rendered as wireframe, else to false.
	*/
	void SetWireframe(const bool wf);
	/*
		Tell the renderstate how it should cull faces relative to the camera.
		@param fc, FaceCulling enum.

		@see RenderState::FaceCulling
	*/
	void SetFaceCulling(const FaceCulling fc);
	/*
		@param depthBuffer, set this to true if objects should be rendered with depthbuffer enabled, else to false.
	*/
	void SetUsingDepthBuffer(const bool depthBuffer);

	void SetAllowTransparency(const bool allowTransparency);

	/*
		@Return true if the render state is using wireframe.
	*/
	bool GetWireframe() const;

	FaceCulling GetFaceCulling() const;
	/*
		@Return true if the render state is using depthbuffer.
	*/
	bool GetIsUsingDepthBuffer() const;

	bool GetAllowTransparency();


protected:
	RenderState();

private:
	bool m_wireframe;
	FaceCulling m_faceCulling;
	bool m_useDepthBuffer;
	bool m_allowTransparency;
};