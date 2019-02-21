#include "Material.hpp"

Material::~Material()
{
}

const MtlData & Material::GetMtlData() const
{
	return mMtlData;
}

Material::Material()
{
}
