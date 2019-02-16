#pragma once
#include "../Mesh.hpp"

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class D3D12Mesh : public Mesh{
public:
	D3D12Mesh();
	// Inherited via Mesh
	virtual bool LoadFromFile(const char * fileName) override;
	virtual void InitializeCube() override;
	virtual void InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) override;
	virtual void InitializePolygonList(const std::vector<Polygon>& polygons) override;
};