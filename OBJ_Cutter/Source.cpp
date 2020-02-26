#include "../D3D12Engine/Loaders/OBJ_Loader.h"
//#include "../D3D12Engine/D"
#include <iostream>
#include "../D3D12Engine/D3D12/External/LodePNG/lodepng.h"
#include <Windows.h>
#include <filesystem>

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
		Barrycoords.z = 1 - (Barrycoords.x + Barrycoords.y);
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
		if (checkVertex(u, texture)) {
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

	Texture texture;
	lodepng::decode(texture.data, texture.w, texture.h, "../../Exported_Assets/Textures/T_WF_TreeBirch_foliage1_NNTS.png");
	std::string mat = "debrisclusterdata_M_WF_TreeBirch_01_foliage";
	
	std::string inPath = "D:/EXJOB/Exported_Assets/Models/SmallTree/BlenderTest/Cut/temp1/";
	std::string outPath = "D:/EXJOB/Exported_Assets/Models/SmallTree/BlenderTest/Cut/temp2/";

	std::filesystem::create_directories(inPath);
	std::filesystem::create_directories(outPath);

	std::string message = "";
	for (auto e : std::filesystem::directory_iterator(inPath))
	{
		//std::string a = e.path().string();
		//std::string b = e.path().extension().string();
		//std::string c = e.path().filename().string();
		//std::string d = e.path().parent_path().string();
		//
		//std::string outName = e.path().filename().string();
		//int ind = outName.find_last_of(".");
		//outName = outName.substr(0, ind);
		
		if (e.path().extension().string() == ".obj") {
			
			std::string outName = outPath + e.path().filename().string();
			p_data.clear();
			n_data.clear();
			uv_data.clear();
			if (!LOADER::LoadOBJ(e.path().string().c_str(), p_data, n_data, uv_data)) {
				return false;
			}
			
			int i = 0;
			i = Cut(texture, p_data[mat], n_data[mat], uv_data[mat]);
			
			LOADER::SaveOBJ(outName.c_str(), p_data, n_data, uv_data);

			message += ("removed " + std::to_string(i) + " triangles from: " + e.path().filename().string()) + "\n";
		}
	}

	if (message == "") {
		message = "No files found.";
	}
	MessageBoxA(NULL, message.c_str(), "Done", 0);

	return 0;
}