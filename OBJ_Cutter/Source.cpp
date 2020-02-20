#include "../D3D12Engine/Loaders/OBJ_Loader.h"
//#include "../D3D12Engine/D"
#include <iostream>
#include "../D3D12Engine/D3D12/External/LodePNG/lodepng.h"
#include <Windows.h>

void subdivide(const Float3* tri, std::vector<Float3>& result, int cur, const int max) {

	Float3 n[3];
	n[0] = (tri[0] + tri[1]) * 0.5f;
	n[1] = (tri[1] + tri[2]) * 0.5f;
	n[2] = (tri[0] + tri[2]) * 0.5f;
	std::vector<Float3> temp;
	/*
			 t0
			 /\
		  n2/__\n0
		   /\  /\
		  /  \/  \
		t2---n1--- t1
	*/

	size_t size = result.size();

	/*
		TOP
		   t0
		   / \
		n2/_x_\n0
		 /\   /\
		/  \ /  \
	  t2---n1--- t1
	*/
	temp.push_back(tri[0]);
	temp.push_back(n[0]);
	temp.push_back(n[2]);

	/*
		Right
		   t0
		   / \
		n2/___\n0
		 /\   /\
		/  \ /x \
	  t2---n1--- t1
	*/
	temp.push_back(tri[1]);
	temp.push_back(n[1]);
	temp.push_back(n[0]);

	/*
		MIDDLE
		   t0
		   / \
		n2/___\n0
		 /\ x /\
		/  \ /  \
	  t2---n1--- t1
	*/
	temp.push_back(n[2]);
	temp.push_back(n[0]);
	temp.push_back(n[1]);

	/*
	LEFT
		   t0
		   / \
		n2/___\n0
		 /\   /\
		/x \ /  \
	  t2---n1--- t1
	*/
	temp.push_back(n[2]);
	temp.push_back(n[1]);
	temp.push_back(tri[2]);

	if (cur < max - 1) {
		for (int i = 0; i < 4; i++) {
			subdivide(&temp[i * 3], result, cur + 1, max);
		}
	}
	else {
		result.insert(result.end(), temp.begin(), temp.end());
	}

	return;
}

struct Texture {
	std::vector<unsigned char> data;
	unsigned int w;
	unsigned int h;
};

unsigned char GetAlpha(const Float2& uv, const Texture& texture) {
	int x = uv.u * texture.w;
	int y = (1 - uv.v) * texture.h;

	x %= texture.w;
	y %= texture.h;

	return texture.data[(x + y * texture.w) * 4 + 3];
}

Float2 Barrypolation(Float3 barry, Float2 in1, Float2 in2, Float2 in3)
{
	return in1 * barry.x + in2 * barry.y + in3 * barry.z;
}

bool checkVertex(const Float2* v_uv, const Texture& texture) {
	for (size_t i2 = 0; i2 < 3; i2++)
	{
		if (GetAlpha(v_uv[i2], texture) > 100) {
			return true;
		}
	}

	if (GetAlpha(Barrypolation(Float3(0.5, 0, 0), v_uv[0], v_uv[1], v_uv[2]), texture) > 100) {
		return true;
	}
	if (GetAlpha(Barrypolation(Float3(0, 0.5, 0), v_uv[0], v_uv[1], v_uv[2]), texture) > 100) {
		return true;
	}
	if (GetAlpha(Barrypolation(Float3(0, 0, 0.5), v_uv[0], v_uv[1], v_uv[2]), texture) > 100) {
		return true;
	}

	Float3 Barrycoords;
	for (size_t i = 0; i < 10; i++)
	{
		Barrycoords.x = rand() / (float)RAND_MAX;
		Barrycoords.y = rand() / (float)RAND_MAX;
		Barrycoords.z = rand() / (float)RAND_MAX;
		if (GetAlpha(Barrypolation(Barrycoords, v_uv[0], v_uv[1], v_uv[2]), texture) > 128) {
			return true;
		}
	}
	
	return false;
}

int Cut(const Texture& texture, std::vector<Float3>& pos, std::vector<Float3>& norms, std::vector<Float2>& uVs) {

	int nRemoved = 0;
	//////////////////////////////////////////

	std::vector<Float3> result_p;
	std::vector<Float3> result_n;
	std::vector<Float2> result_u;

	int i = 0;
	size_t size = pos.size();
	Float3* v = &pos[0];
	Float3* n = &norms[0];
	Float2* u = &uVs[0];

	while (i < size)
	{
		bool keep = false;
		for (size_t i2 = 0; i2 < 3; i2++)
		{
			if (GetAlpha(u[i2], texture) > 100) {
				keep = true;
				break;
			}
		}

		if (keep) {
			for (size_t i2 = 0; i2 < 3; i2++)
			{
				result_p.push_back(v[i2]);
				result_n.push_back(n[i2]);
				result_u.push_back(u[i2]);
			}
		}
		else {
			nRemoved++;
		}

		i += 3;
		v += 3;
		n += 3;
		u += 3;
	}

	pos = result_p;
	norms = result_n;
	uVs = result_u;

	return nRemoved;
}

int main() {
	LOADER::FLOAT3_BUFFER p_data;
	LOADER::FLOAT3_BUFFER n_data;
	LOADER::FLOAT2_BUFFER uv_data;

	if (!LOADER::LoadOBJ("../../Exported_Assets/Models/Small_Tree_subdivided.obj", p_data, n_data, uv_data)) {
		return false;
	}

	//LOADER::SaveOBJ("../../Exported_Assets/Models/cover_cutted_orig.obj", material_facePositions, material_faceNormals, material_faceUVs);

	Texture texture;
	lodepng::decode(texture.data, texture.w, texture.h, "../../Exported_Assets/Textures/T_WF_TreeBirch_foliage1_NNTS.png");

	std::string mat = "debrisclusterdata_M_WF_TreeBirch_01_foliage";
	
	int i = 0;
	i = Cut(texture, p_data[mat], n_data[mat], uv_data[mat]);

	LOADER::SaveOBJ("../../Exported_Assets/Models/Small_Tree_subdivided_cut.obj", p_data, n_data, uv_data);

	MessageBoxA(NULL, (std::to_string(i) + " triangles was removed!").c_str(), "Done", 0);

	//material_facePositions.clear();
	//material_faceNormals.clear();
	//material_faceUVs.clear();
	//if (!LOADER::LoadOBJ("../../Exported_Assets/Models/cover_cutted.obj", material_facePositions, material_faceNormals, material_faceUVs)) {
	//	return false;
	//}

	return 0;
}