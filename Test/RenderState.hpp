#pragma once
/*
	Specific rendering states goes here like Wireframe, conservative rasterization etc.
*/
class RenderState {
protected:
	RenderState();
private:
	bool wireframe;
	//bool useDepthBuffer; //Should this be here maybe?
};