#pragma once
#include "../Mesh.hpp"

class D3D12VertexBuffer;
class D3D12Renderer;

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class D3D12Mesh : public Mesh{
public:
	D3D12Mesh(D3D12Renderer* renderer);
	virtual ~D3D12Mesh();

	// Note: this is limited to only reading triangles as of now
	virtual bool LoadFromFile(const char * fileName) override;

	virtual bool InitializeCube(unsigned int vertexBufferFlags) override;
	virtual bool InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) override;
	virtual bool AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType) override;

	std::vector<D3D12VertexBuffer*>* GetVertexBuffers();
private:
	D3D12Renderer* renderer;
	std::vector<D3D12VertexBuffer*> vertexBuffers;
};