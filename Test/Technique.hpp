#pragma once

class Material;
class RenderState;

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class Technique {
protected:
	Technique(Material*, RenderState*);
private:
	Material* mat;
	RenderState* renderState;
};