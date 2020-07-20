#include "stdafx.h"
#include "ConfigFileLoader.hpp"

namespace ConfigParser {
	using namespace ConfigLoader;

	class ParseExpression : virtual public RegularExp::Expression {
	public:
		virtual int Check(char*& current, ConfigLoader::ConfigTreeNode* parrentNode) const = 0;
	private:
		virtual int Check(char*& current, void* parrentNode) const override;
	};

	class Setting : public ParseExpression {
	public:
		Setting() {};
		~Setting() {};
		// Inherited via Expression
		virtual int Check(char*& current, ConfigTreeNode* parrentNode) const override;
	};
	const ConfigParser::Setting g_setting;

	class SettingName : public ParseExpression {
	public:
		SettingName() {};
		~SettingName() {};
		// Inherited via Expression
		virtual int Check(char*& current, ConfigTreeNode* parrentNode) const override;
	};
	const ConfigParser::SettingName g_setting_name;

	class SettingValue : public ParseExpression {
	public:
		SettingValue() {};
		~SettingValue() {};
		// Inherited via Expression
		virtual int Check(char*& current, ConfigTreeNode* parrentNode) const override;
	};
	const ConfigParser::SettingValue g_setting_value;

	class Array : public ParseExpression {
	public:
		Array() {};
		~Array() {};
		// Inherited via Expression
		virtual int Check(char*& current, ConfigTreeNode* parrentNode) const override;
	};
	const ConfigParser::Array g_array;
	RegularExp::AND g_nextArrayValue({ &RegularExp::Special::g_seperator_star, &RegularExp::Special::g_comma, &RegularExp::Special::g_seperator_star, &g_setting_value });
	RegularExp::Star g_nextArrayValue_star(g_nextArrayValue);

	class InlineComment : public ParseExpression {
	public:
		InlineComment() {};
		~InlineComment() {};
		// Inherited via Expression
		virtual int Check(char*& current, ConfigTreeNode* parrentNode) const override;
	};
	const ConfigParser::InlineComment g_inline_comment;

}

////////////////////////////////////////////////////
int ConfigParser::ParseExpression::Check(char*& current, void* parrentNode) const {
	return Check(current, (ConfigTreeNode*)parrentNode);
}

int ConfigParser::Setting::Check(char*& current, ConfigTreeNode* parrentNode) const {
	const char* start = current;
	ConfigTreeNode* myNode = MY_NEW ConfigTreeNode;
	myNode->type = ConfigTreeNodeType::Setting;

	//Detect the setting name, if found it is pushed to myNode
	if (g_setting_name.Check(current, myNode) > 0) {
		//Detect the equal sign with optional space characters before and after.
		if (RegularExp::Special::g_seperator_star.Check(current, myNode) >= 0 && RegularExp::Special::g_equal_token.Check(current, myNode) > 0 && RegularExp::Special::g_seperator_star.Check(current, myNode) >= 0) {
			//Detect the setting value, if found it is pushed to myNode
			if (g_array.Check(current, myNode) > 0) {
				//The value is an array
				parrentNode->subnodes.push_back(myNode);
				return current - start;
			} else if (g_setting_value.Check(current, myNode) > 0) {
				//Setting name and value found. Keep looking forward to make sure that the row is consumed
				if (g_inline_comment.Check(current, myNode) >= 0) {
					//if the Setting pattern is correct, push myNode to parrentNode and return
					parrentNode->subnodes.push_back(myNode);
					return current - start;
				} else {
					//There was more data after the value. Something is not right with the parsed data.
				}
			}
		}
	}

	//if the pattern failed, revert all changes and return -1 
	myNode->Delete();
	current = (char*)start;
	return -1;
}

int ConfigParser::SettingName::Check(char*& current, ConfigTreeNode* parrentNode) const {
	const char* start = current;
	ConfigTreeNode* myNode = MY_NEW ConfigTreeNode;
	myNode->type = ConfigTreeNodeType::String;

	int i;
	if ((i = RegularExp::Strings::g_string_letters_digits.Check(current, myNode)) < 0) {
		//if the pattern failed, revert all changes and return -1
		myNode->Delete();
		current = (char*)start;
		return -1;
	}
	myNode->value = std::string(start, i);
	parrentNode->subnodes.push_back(myNode);

	return i;
}

int ConfigParser::SettingValue::Check(char*& current, ConfigTreeNode* parrentNode) const {
	const char* start = current;
	ConfigTreeNode* myNode = MY_NEW ConfigTreeNode;

	int i;
	if ((i = RegularExp::Strings::g_string_letters_tokens_digits.Check(current, myNode)) > 0) {
		//If value is a string
		myNode->type = ConfigTreeNodeType::String;
	} else if ((i = RegularExp::Numbers::g_numberPositive.Check(current, myNode)) > 0) {
		//If value is an integer
		myNode->type = ConfigTreeNodeType::Integer;
	} else {
		//if the pattern failed, revert all changes and return -1
		myNode->Delete();
		current = (char*)start;
		return -1;
	}

	myNode->value = std::string(start, i);
	parrentNode->subnodes.push_back(myNode);
	return i;

}

bool ConfigLoader::Load(const char* fileName, ConfigLoader::ConfigTreeNode& rootNode, std::string* error_message) {
	std::ifstream in_file(fileName);
	if (!in_file.is_open()) {
		if (error_message) {
			*error_message = "ConfigLoader failed to parse: " + std::string(fileName) + ". File not found!";
		}
		return false;
	}

	std::string line;
	char* line_data_ptr;
	int row = 0;
	bool falied = false;
	while (std::getline(in_file, line) && !falied) {
		row++;
		line_data_ptr = line.data();
		if (line_data_ptr[0] == '#' || line_data_ptr[0] == '\0') {
			continue;
		} else if (ConfigParser::g_setting.Check(line_data_ptr, &rootNode) < 0) {
			//Failed
			falied = true;
			break;
		}

	}

	in_file.close();

	if (falied) {
		if (error_message) {
			*error_message = "ConfigLoader failed to parse: " + std::string(fileName) + " at line " + std::to_string(row) + "( \"" + line + "\" )";
		}
		return false;
	}
	return true;
}

bool ConfigLoader::Save(const char* fileName, ConfigTreeNode& configRoot, std::string* error) {
	std::ofstream out_file(fileName);
	if (!out_file.is_open()) {
		if (error) {
			*error = "ConfigLoader failed to save: " + std::string(fileName) + ". File could not be opened!";
		}
		return false;
	}

	std::string s;
	configRoot.ToString(s);
	out_file << s;
	out_file.close();

	return true;
}

int ConfigParser::InlineComment::Check(char*& current, ConfigTreeNode* parrentNode) const {
	const char* start = current;
	//Skip all spaces and tabs
	RegularExp::Special::g_seperator_star.Check(current, nullptr);

	if (RegularExp::Special::g_hashtag_token.Check(current, nullptr) > 0) {
		//If it is a comment, consume the rest of the line.
		RegularExp::Special::g_not_EOL_star.Check(current, parrentNode);

		return current - start;
	} else if (current[0] == '\0' || current[0] == '\n') {
		//There was no more data. This can count as a comment with size 0, or be treated be a fail by the caller.
		current = (char*)start;
		return 0;
	} else {
		//There was more data. This can definitely not count as a comment.
		current = (char*)start;
		return -1;
	}


}

int ConfigParser::Array::Check(char*& current, ConfigTreeNode* parrentNode) const {
	const char* start = current;
	ConfigTreeNode* myNode = MY_NEW ConfigTreeNode;
	myNode->type = ConfigTreeNodeType::Array;

	//Detect Begining Of Array "{"
	if (RegularExp::Special::g_curly_start.Check(current, myNode) >= 0) {
		//Detect array elements. Find one value followed by any number of comma seperated values.
		if (ConfigParser::g_setting_value.Check(current, myNode) >= 0 && g_nextArrayValue_star.Check(current, myNode) >= 0) {
			//Detect End Of Array "}"
			if (RegularExp::Special::g_curly_end.Check(current, myNode) >= 0) {
				parrentNode->subnodes.push_back(myNode);
				return current - (char*)start;
			}
		}
	}

	myNode->Delete();
	return -1;
}

void ConfigLoader::ConfigTreeNode::Print_Depth(std::vector<bool>& b, int depth) {
	for (int i = 1; i < depth; i++) {
		if (!b[i]) {
			std::cout << "    ";
		} else {
			std::cout << "|   ";
		}
	}

	if (depth > 0) {
		std::cout << "|---";
	}
}

void ConfigLoader::ConfigTreeNode::Print(std::vector<bool>& b, int depth) {
	int childSkip = 0;
	switch (type) {
	default:
		Print_Depth(b, depth);
		std::cout << ConfigLoader::ConfigTreeNodeType_String[(int)type] << " " << value << std::endl;
		break;
	}

	int n = 0;
	int s = subnodes.size();
	b.emplace_back(false);
	for (auto& e : subnodes) {
		if (n++ < childSkip) { continue; }
		b.back() = (n != s);
		e->Print(b, depth + 1);
	}
	b.erase(b.begin() + b.size() - 1);
}

void ConfigLoader::ConfigTreeNode::ToString(std::string& s) {
	size_t size = 0;
	size_t i = 0;
	switch (type) {
	case ConfigLoader::ConfigTreeNodeType::Root:
		s = "";
		size = subnodes.size();
		i = 0;
		for (auto& e : subnodes) {
			e->ToString(s);
			if (++i < size) {
				s += "\n";
			}
		}
		break;
	case ConfigLoader::ConfigTreeNodeType::Setting:
		subnodes[0]->ToString(s);
		s += " = ";
		subnodes[1]->ToString(s);
		break;
	case ConfigLoader::ConfigTreeNodeType::Array:
		s += "{";
		size = subnodes.size();
		i = 0;
		for (auto& e : subnodes) {
			e->ToString(s);
			if (++i < size) {
				s += ",";
			}
		}
		s += "}";
		break;
	case ConfigLoader::ConfigTreeNodeType::String:
	case ConfigLoader::ConfigTreeNodeType::Integer:
		s += value;
		break;
	default:
		s += "#Save not implemented for node type: \"" + ConfigTreeNodeType_String[(int)type] + "\"";
		break;
	}
}

ConfigLoader::ConfigTreeNode* ConfigLoader::ConfigTreeNode::operator [](int i) {
	if (i < subnodes.size()) {
		return nullptr;
	} else {
		return subnodes[i];
	}
}

ConfigLoader::ConfigTreeNode* ConfigLoader::ConfigTreeNode::operator[](std::string settingName) {
	//Slow. TODO: Make Fast!
	for (auto& e : subnodes) {
		if (e->type == ConfigTreeNodeType::Setting) {
			if (e->subnodes.size() >= 2 && e->subnodes[0]->value == settingName) {
				return e->subnodes[1];
			}
		}
	}
	return nullptr;
}


void ConfigLoader::ConfigTreeSettingNode::SetName(std::string name) {
	subnodes[0]->value = name;
}

bool ConfigLoader::ConfigTreeSettingNode::SetValue(ConfigTreeNode*& value) {
	if (value == nullptr) {
		return false;
	}
	if (value->type == ConfigTreeNodeType::String || value->type == ConfigTreeNodeType::Integer || value->type == ConfigTreeNodeType::Array) {
		subnodes[1]->Delete();
		subnodes.pop_back();
		subnodes.push_back(value);
		value = nullptr;
		return true;
	}
	return false;
}

bool ConfigLoader::ConfigTreeValueArrayNode::AddElement(int element) {
	ConfigTreeNode* node = new ConfigTreeNode;
	node->type = ConfigTreeNodeType::Integer;
	node->value = std::to_string(element);
	subnodes.push_back(node);
	return true;
}

bool ConfigLoader::ConfigTreeValueArrayNode::AddElement(std::string element) {
	ConfigTreeNode* node = new ConfigTreeNode;
	node->type = ConfigTreeNodeType::String;
	node->value = element;
	subnodes.push_back(node);
	return true;
}
