#include "D3D12Material.hpp"
#include <sstream>
#include <fstream>

D3D12Material::D3D12Material()
{
}

D3D12Material::~D3D12Material()
{
}

bool D3D12Material::LoadFromFile(const char * fileName)
{
	if (!fileName)
		return false;

	std::string fullPath = std::string("../assets/Materials/") + fileName;
	std::ifstream inFile(fullPath);
	if (!inFile.is_open())
		return false;

	std::string line, type, temp;
	
	/*
	Copy data from file to material
	*/
	while (std::getline(inFile, line))
	{
		type = "";

		std::istringstream iss(line);

		iss >> type;

		if (type == "newmtl")
		{
			iss >> mMtlData.materialName;
		}
		else if (type == "illum")
		{
			iss >> mMtlData.illuminationModel;
		}
		else if (type == "Kd")
		{
			iss >> mMtlData.diffuseReflectivity.x >> mMtlData.diffuseReflectivity.y >> mMtlData.diffuseReflectivity.z;
		}
		else if (type == "Ka")
		{
			iss >> mMtlData.ambientReflectivity.x >> mMtlData.ambientReflectivity.y >> mMtlData.ambientReflectivity.z;
		}
		else if (type == "Ks")
		{
			iss >> mMtlData.specularReflectivity.x >> mMtlData.specularReflectivity.y >> mMtlData.specularReflectivity.z;
		}
		else if (type == "Tf")
		{
			iss >> mMtlData.transmissionFilter.x >> mMtlData.transmissionFilter.y >> mMtlData.transmissionFilter.z;
		}
		else if (type == "map_Kd")
		{
			iss >> temp;
			mMtlData.defaultTextureNames.push_back(temp);
		}
		else if (type == "Ni")
		{
			iss >> mMtlData.opticalDensity;
		}
	}

	inFile.close();

	return true;
}
