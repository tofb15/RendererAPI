#pragma once

enum class ShaderType {
	UNKNOWN,
	VS,
	GS,
	FS,
};

struct ShaderDescription {
	ShaderType type;
	const char* name;
	const char* defines;
};

struct Shader {
	ShaderType type;
	int handle;
};

struct ShaderProgram {
	Shader VS;
	Shader GS;
	Shader FS;
};

class ShaderManager
{
public:

	virtual ~ShaderManager();

	/*
		@return Shader index, -1 if the shader could not compile
	*/
	virtual Shader CompileShader(const ShaderDescription& sd) = 0;

	/*
		@return Program index, -1 if the program could not be created
	*/
	//virtual int CreateShaderProgram(Shader VS, Shader GS, Shader PS) = 0;
protected:
	ShaderManager();

private:

};