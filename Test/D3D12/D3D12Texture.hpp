#pragma once

#include "../Texture.hpp"
#include "D3D12TextureLoader.hpp"

#include <vector>

class D3D12Renderer;
class D3D12TextureLoader;

struct ID3D12Resource;
//struct ID3D12DescriptorHeap;

/*
	Contain a texture that could be applied to a mesh.
*/
class D3D12Texture : public Texture {
public:
	D3D12Texture(D3D12Renderer* renderer, unsigned short index);
	virtual ~D3D12Texture();

	// Inherited via Texture
	virtual bool LoadFromFile(const char* fileName, unsigned flags) override;
	bool IsLoaded();
	int GetTextureIndex() const;
	void UpdatePixel(Int2 pos, const unsigned char* data, int size) override;
	void ApplyChanges() override;

	std::vector<unsigned char>& GetData_addr();
	const std::vector<unsigned char>& GetData_addr_const() const;
	std::vector<unsigned char> GetData_cpy() const;
private:
	friend D3D12TextureLoader;

	//void load();
	//bool CreateGPUTextureResource();

	bool m_IsLoaded = false;
	bool m_hasChanged = false;

	std::vector<unsigned char> m_Image_CPU; //the raw pixels stored on the CPU.
	D3D12Renderer* m_Renderer;

	/*Used by the Texture loader to find the right GPU address for this specific texture*/
	int m_GPU_Loader_index = -1;

	//ID3D12Resource* mResource;
	//ID3D12DescriptorHeap* mDescriptorHeap;
};