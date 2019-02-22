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
	D3D12Texture(D3D12Renderer* renderer);
	virtual ~D3D12Texture();

	// Inherited via Texture
	virtual bool LoadFromFile(const char* fileName, unsigned flags) override;
	bool IsLoaded();
private:
	friend D3D12TextureLoader;

	void load();
	bool CreateGPUTextureResource();


	bool mIsLoaded = false;
	std::vector<unsigned char> mImage_CPU; //the raw pixels stored on the CPU.
	unsigned mFlags;
	D3D12Renderer* mRenderer;
	ID3D12Resource* mResource;
	//ID3D12DescriptorHeap* mDescriptorHeap;
};