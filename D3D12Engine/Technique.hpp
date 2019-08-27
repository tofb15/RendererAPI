#pragma once

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class Technique {
public:
	virtual ~Technique();
	virtual bool Enable() = 0;

protected:
	Technique();
private:
};