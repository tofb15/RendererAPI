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
	void SetWireframe(const bool wf);	
	void SetFaceCulling(const FaceCulling fc);

	bool GetWireframe() const;
	FaceCulling GetFaceCulling() const;

protected:
	RenderState();

private:
	bool m_wireframe;
	FaceCulling m_faceCulling;

	//bool useDepthBuffer; //Should this be here maybe?
};