#pragma once
#include <string>
enum class ShaderType {
	UNKNOWN = 0,
	//Raster
	VS,
	GS,
	FS,
	//DXR
	INTERSECTION,
	ANY_HIT,
	CLOSEST_HIT,
};

struct ShaderDescription {
	ShaderType type = ShaderType::UNKNOWN;
	std::wstring filename;
	std::wstring entrypoint;
	std::wstring defines;
};

typedef int ShaderHandle;
typedef int ShaderProgramHandle;

struct ShaderProgramDescription {
	ShaderHandle VS = -1;//Vertex Shader
	ShaderHandle GS = -1;//Geometry Shader
	ShaderHandle FS = -1;//Fragment Shader
	//DXR		   
	ShaderHandle IS = -1; //Intersection Shader
	ShaderHandle AHS = -1;//Any-hit Shader
	ShaderHandle CHS = -1;//Closest Shader
};



class ShaderManager {
public:

	virtual ~ShaderManager();

	virtual ShaderHandle RegisterShader(const ShaderDescription& shaderDescription) = 0;
	virtual ShaderProgramHandle RegisterShaderProgram(const ShaderProgramDescription& shaderDescription) = 0;
	virtual void RecompileShaders() = 0;
protected:
	ShaderManager();

private:

};