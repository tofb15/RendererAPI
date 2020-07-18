#pragma once
#include <string>
#include <vector>
#include "../D3D12Engine/Utills/RegularExpressions.h"

#include <fstream>
#include <iostream>

namespace ConfigLoader {
	enum class ConfigTreeNodeType {
		Root = 0,
		Setting,
		Array,
		String,
		Integer,
	};
	const std::string ConfigTreeNodeType_String[] = {
		"Root",
		"Setting",
		"Array",
		"String",
		"Integer",
	};

	class ConfigTreeNode {
	public:
		ConfigTreeNode() {
			type = ConfigTreeNodeType::Root;
		};
		void Delete() { delete this; }
		ConfigTreeNodeType type;
		std::string value;
		std::vector<ConfigTreeNode*> subnodes;

		void Print_Depth(std::vector<bool>& b, int depth);
		void Print(std::vector<bool>& b, int depth = 0);
	protected:
		//This class must be created on the heap using new. Stack allocation is prohibited.
		~ConfigTreeNode() {
			for (auto& e : subnodes) {
				delete e;
			}
		}
	};

	bool Load(const char* fileName, ConfigTreeNode& configRoot, std::string* error = nullptr);
}



