#pragma once
#include "FusionReactor/src/Mesh.hpp"
#include <unordered_map>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12VertexBuffer;
		class D3D12API;

		/*
			Used to contain a model.
			Basicly just a collection of related vertexbuffers.
		*/
		class D3D12Mesh : public Mesh {
		public:
			D3D12Mesh(D3D12API* renderer, unsigned short id);
			virtual ~D3D12Mesh();

			// Note: this is limited to only reading triangles as of now
			virtual bool LoadFromFile(const char* fileName, MeshLoadFlag loadFlag) override;
			virtual bool InitializeCube(unsigned int vertexBufferFlags) override;
			virtual bool InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) override;
			virtual int GetNumberOfSubMeshes() override;
			virtual std::string GetSubMesheName(int i) override;

			virtual D3D12VertexBuffer* GetVertexBuffer(VertexBufferFlag bufferType);
			virtual std::unordered_map<std::string, std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*>>& GetSubObjects();

			virtual bool CalculateTangentAndBinormal(Float3* positions, Float2* uvs, int nElements, std::string subObject);

			std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*>* GetVertexBuffers();
			std::vector<D3D12VertexBuffer*> GetVertexBuffers_vec();

			unsigned short GetID() const;

		private:
			virtual bool AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType, std::string subObject);
			virtual bool AddVertexBuffer(D3D12VertexBuffer* buffer, Mesh::VertexBufferFlag bufferType, std::string subObject);

		private:
			unsigned short m_id;
			D3D12API* m_d3d12;
			//std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*> m_vertexBuffers;
			std::unordered_map<std::string, std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*>> m_subObjects;
		};
	}
}