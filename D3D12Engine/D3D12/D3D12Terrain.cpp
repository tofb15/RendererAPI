#include "stdafx.h"

#include "D3D12Terrain.hpp"
#include "D3D12Texture.hpp"
#include "D3D12API.hpp"

D3D12Terrain::D3D12Terrain(D3D12API * renderer) : Terrain()
{
	m_mesh = renderer->MakeMesh();
}

D3D12Terrain::~D3D12Terrain()
{
	if(m_mesh)
		delete m_mesh;
}

bool D3D12Terrain::InitializeHeightMap(Texture * _texture, float maxHeight)
{
	D3D12Texture* texture = dynamic_cast<D3D12Texture*>(_texture);
	if (texture == nullptr)
		return false;

	if (!(texture->GetFlags() & Texture::TEXTURE_USAGE_CPU_FLAG))
		return false;

	unsigned bpp = texture->GetBPP();
	float scaleY = maxHeight / 255.0f;

	const std::vector<unsigned char> &textureData = texture->GetData_addr_const();
	size_t size = textureData.size();

	unsigned width = texture->GetWidth();
	int width_half = width * 0.5;
	unsigned height = texture->GetHeight();
	int height_half = height * 0.5;
	unsigned verticesNeeded = (width - 1) * (height - 1) * 6;

	Float3* data = MY_NEW Float3[verticesNeeded];
	int dataPoint = 0;

	float posScale = 5;

	for (unsigned y = 0; y < height - 1; y++)
	{
		unsigned y0_width = y * width;
		for (unsigned x = 0; x < width - 1; x++)
		{



			/*
			1	2
			.__.
			| /
			./
			3
			*/
			data[dataPoint++] = Float3((float)x * posScale,		(float)((float)textureData[(y0_width + x)*bpp]), -(float)y * posScale);				// 1
			data[dataPoint++] = Float3((float)(x + 1) * posScale,	(float)((float)textureData[(y0_width + x + 1)*bpp]), -(float)y * posScale);			// 2
			data[dataPoint++] = Float3((float)x * posScale,		(float)((float)textureData[(y0_width + width + x)*bpp]), -((float)y + 1) * posScale);		// 3

			/*
			 	4
			   .
			  /|
			./_.
			6	5
			*/
			data[dataPoint++] = Float3((float)(x + 1) * posScale,	(float)((float)textureData[(y0_width + x + 1)*bpp]), -(float)y * posScale);			// 4
			data[dataPoint++] = Float3((float)(x + 1) * posScale,	(float)((float)textureData[(y0_width + width + x + 1)*bpp]), -(float)(y + 1)* posScale); // 5
			data[dataPoint++] = Float3((float)x * posScale,		(float)((float)textureData[(y0_width + width + x)*bpp]), -(float)(y + 1)* posScale);		// 6
		}
	}

	//Create vertexbuffer for normals
	//if (!m_mesh->AddVertexBuffer(verticesNeeded, sizeof(Float3), data, Mesh::VERTEX_BUFFER_FLAG_POSITION, "Terrain"))
	//	return false;

	delete[] data;

	return true;
}
