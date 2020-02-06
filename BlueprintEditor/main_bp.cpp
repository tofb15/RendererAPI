
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

#include "..\D3D12Engine\ResourceManager.h"

std::vector<BlueprintDescription> m_bpDescriptions;

bool InitializeBlueprints()
{
	m_bpDescriptions.emplace_back(BlueprintDescription{"map",       "map.obj",      true, {"ConcreteCover_CS.png", "emplyNormal.png"} });
	m_bpDescriptions.emplace_back(BlueprintDescription{"tree",      "testTree.obj", true, {"Tree_C.png", "Tree_N.png"} });
	m_bpDescriptions.emplace_back(BlueprintDescription{"concrete",  "stone.obj",    true, {"ConcreteCover_CS.png", "ConcreteCover_NX.png"} });
	m_bpDescriptions.emplace_back(BlueprintDescription{"sandbag",   "cover.obj",    true, {"Sandbag_CS.png", "Sandbag_N.png"} });
	m_bpDescriptions.emplace_back(BlueprintDescription{"floor",     "floor.obj",    true, {"Floor_CS.png", "Floor_N.png"} });
	m_bpDescriptions.emplace_back(BlueprintDescription{"tent",      "Tent.obj",     false,{"T_Poles_CS.png", "T_Poles_NX.png", "T_Net_CS.png", "T_Net_NA.png"} });
	return true;
}

int main() {
	InitializeBlueprints();
	ResourceManager::SaveBlueprintToFile(m_bpDescriptions);
	return 0;
}