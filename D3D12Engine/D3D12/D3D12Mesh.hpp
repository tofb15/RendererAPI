#pragma once
#include "../Mesh.hpp"

class D3D12VertexBuffer;
class D3D12API;

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class D3D12Mesh : public Mesh{
public:
	D3D12Mesh(D3D12API* renderer, unsigned short id);
	virtual ~D3D12Mesh();

	// Note: this is limited to only reading triangles as of now
	virtual bool LoadFromFile(const char * fileName) override;
	virtual bool InitializeCube(unsigned int vertexBufferFlags) override;
	virtual bool InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) override;
	virtual bool AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType) override;

	virtual bool CalculateTangentAndBinormal(Float3* positions, Float2* uvs, int nElements);

	std::vector<D3D12VertexBuffer*>* GetVertexBuffers();
	unsigned short GetID() const;
private:
	unsigned short m_id;
	D3D12API* m_renderer;
	std::vector<D3D12VertexBuffer*> m_vertexBuffers;
};