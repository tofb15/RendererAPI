#include "D3D12ShaderManager.hpp"
#include <d3dcompiler.h>
#include <fstream>
#include "D3D12Renderer.hpp"

#pragma comment(lib, "d3dcompiler.lib")

D3D12ShaderManager::D3D12ShaderManager(D3D12Renderer * renderer)
{
	mRenderer = renderer;
}

D3D12ShaderManager::~D3D12ShaderManager()
{
	for (auto& blobList : mShader_blobs)
	{
		for (auto& blob : blobList.second)
		{
			blob->Release();
		}
	}
}

Shader D3D12ShaderManager::CompileShader(ShaderDescription sd)
{
	std::vector<ID3DBlob*> &shaderVector = mShader_blobs[sd.type];

	if(sd.type == ShaderType::VS)
		mVertexDefines.push_back(sd.defines);

	ID3DBlob* blob;

	HRESULT hr;

	std::string shaderPath = "../assets/D3D12/" + std::string(sd.name) + ".hlsl";
	std::ifstream shaderFile(shaderPath);

	std::string shaderText;
	std::string completeShaderText("");
	std::string entry = "main";
	std::string model;

	// Set entry point and model based on type
	switch (sd.type)
	{
	case ShaderType::VS: model = "vs_5_0"; break;
	case ShaderType::FS: model = "ps_5_0"; break;
	case ShaderType::GS: model = "gs_5_0"; break;
	//case ShaderType::CS: entry = "main"; model = "cs_5_0"; break;
	default: break;
	}

	// open the file and read it to a string "shaderText"
	if (shaderFile.is_open())
	{
		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
		shaderFile.close();
	}
	else
	{
		return { ShaderType::UNKNOWN , -1 };
	}

	// Create the complete shader text (defines + file data)
	//completeShaderText += sd.defines;
	completeShaderText += shaderText;

	hr = D3DCompile(
		completeShaderText.c_str(),
		completeShaderText.size(),
		nullptr,
		nullptr,
		nullptr,
		entry.c_str(),
		model.c_str(),
		0U,
		0U,
		&blob,
		nullptr
	);
	if (FAILED(hr))
	{
		return { ShaderType::UNKNOWN , -1 };
	}

	shaderVector.push_back(blob);

	return { sd.type, static_cast<int>(shaderVector.size()) - 1 };
}

std::string D3D12ShaderManager::GetVertexDefines(int index) const
{
	return mVertexDefines[index];
}

ID3DBlob * D3D12ShaderManager::GetShaderBlob(Shader shader)
{
	return mShader_blobs[shader.type][shader.handle];
}
