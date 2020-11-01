#pragma once
#include <string>
#include <vector>
#include "Utills/RegularExpressions.h"

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
		void ToString(std::string& s);

		ConfigTreeNode* operator [](int i);
		ConfigTreeNode* operator [](std::string settingName);

	protected:
		//This class must be created on the heap using new. Stack allocation is prohibited.
		~ConfigTreeNode() {
			for (auto& e : subnodes) {
				delete e;
			}
		}
	};

	class ConfigTreeValueArrayNode : public ConfigTreeNode {
		ConfigTreeValueArrayNode() {
			type = ConfigTreeNodeType::Array;
		};
		//Add an element to the array. The element type can be String or Integer.
		bool AddElement(int element);
		bool AddElement(std::string element);

	};

	class ConfigTreeSettingNode : public ConfigTreeNode {
		ConfigTreeSettingNode() {
			type = ConfigTreeNodeType::Setting;
			subnodes.push_back(new ConfigTreeNode);
			subnodes.push_back(new ConfigTreeNode);
			subnodes[1]->type = ConfigTreeNodeType::String;
		};
		//Set the name of the setting
		void SetName(std::string name);
		//Set the value of the setting. The value type can be String, Integer or Array.
		bool SetValue(ConfigTreeNode*& value);
	};

	bool Load(const char* fileName, ConfigTreeNode& configRoot, std::string* error = nullptr);
	bool Save(const char* fileName, ConfigTreeNode& configRoot, std::string* error = nullptr);

}



