#pragma once
#include "../ShaderManager.hpp"
#include <vector>
#include <map>

#include <d3d12.h>

class D3D12Renderer;

class D3D12ShaderManager : public ShaderManager
{
public:

	D3D12ShaderManager(D3D12Renderer* renderer);
	virtual ~D3D12ShaderManager();

	// Inherited via ShaderManager
	virtual Shader CompileShader(ShaderDescription sd) override;
	//virtual int CreateShaderProgram(Shader VS, Shader GS, Shader PS) override;

	std::string GetVertexDefines(int index) const;
	ID3DBlob* GetShaderBlob(Shader shader);
protected:

	std::map<ShaderType, std::vector<ID3DBlob*>> mShader_blobs;
	std::vector<std::string> mVertexDefines;

	D3D12Renderer* mRenderer;

private:



};