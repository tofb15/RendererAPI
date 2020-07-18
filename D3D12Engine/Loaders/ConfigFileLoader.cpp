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
	ConfigTreeNode* myNode = new ConfigTreeNode;
	myNode->type = ConfigTreeNodeType::Setting;

	//Detect the setting name, if found it is pushed to myNode
	if (g_setting_name.Check(current, myNode) > 0) {
		//Detect the equal sign with optional space characters before and after.
		if (RegularExp::Special::g_seperator_star.Check(current, myNode) >= 0 && RegularExp::Special::g_equal_token.Check(current, myNode) > 0 && RegularExp::Special::g_seperator_star.Check(current, myNode) >= 0) {
			//Detect the setting value, if found it is pushed to myNode
			if (g_setting_value.Check(current, myNode) > 0) {
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
	ConfigTreeNode* myNode = new ConfigTreeNode;
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
	ConfigTreeNode* myNode = new ConfigTreeNode;

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
