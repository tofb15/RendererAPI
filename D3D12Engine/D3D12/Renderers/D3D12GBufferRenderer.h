#pragma once
#include "D3D12Renderer.h"

class D3D12GBufferRenderer : public D3D12Renderer {
public:
	// Inherited via D3D12Renderer
	virtual bool Initialize() override;
	virtual void Submit(const SubmissionItem& item, Camera* camera = nullptr, unsigned char layer = 0) override;
	virtual void ClearSubmissions() override;
	virtual void Frame(Window* window, Camera* camera) override;
	virtual void Present(Window* window, GUI* gui = nullptr) override;
	virtual void ClearFrame() override;
	virtual void SetLightSources(const std::vector<LightSource>& lights) override;

private:
	struct SortingItem {
		SortingItem() {

		}
		SortingItem(const SortingItem& other) {
			sortingIndex = other.sortingIndex;
			item = other.item;
		}
		SortingItem& operator=(const SortingItem& other) {
			sortingIndex = other.sortingIndex;
			item = other.item;
			return *this;
		}

		union {
			UINT128 sortingIndex;						//16 Bytes
			struct {
				//By setting meshIndex and techniqueIndex, sortingIndex will be set automatically.
				unsigned short distance;				//2 Bytes
				unsigned short textureIndex;			//2 Bytes
				unsigned short meshIndex;				//2 Bytes
				unsigned short meshTypeDistance;		//2 Bytes, closest element in the mesh
				unsigned short techniqueIndex;			//2 Bytes
				unsigned short techniqueTypeDistance;	//2 Bytes, closest element in the technique
				unsigned char layer;
			};

		};
		//unsigned short techniqueIndex;
		//unsigned short meshIndex;
		SubmissionItem item;
	};

private:
	std::vector<SortingItem> m_items;

	unsigned short m_closestMeshType_float[100];
	unsigned short m_closestMeshType[100];
	unsigned short m_closestMeshType_lastFrame[100];

	unsigned short m_closestTechniqueType_float[100];
	unsigned short m_closestTechniqueType[100];
	unsigned short m_closestTechniqueType_lastFrame[100];

	//Resources
	ID3D12CommandAllocator* m_commandAllocators[NUM_GPU_BUFFERS] = { nullptr };
	ID3D12GraphicsCommandList4* m_commandLists[NUM_GPU_BUFFERS] = { nullptr };

	//RenderTargets
	ID3D12Resource* m_outputTextures[NUM_GPU_BUFFERS] = { nullptr };
	Int2 m_outputDim;
};