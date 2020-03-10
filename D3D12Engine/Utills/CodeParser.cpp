#include "stdafx.h"
#include "CodeParser.h"

// Inherited via Expression
int CodeParse::DataType::Check(char*& current, ParseTreeNode* parrentNode) const {
	const char* start = current;

	ParseTreeNode* myNode = new ParseTreeNode;
	myNode->type = ParseTreeNodeType_DataType;

	if (pars_name.Check(current, myNode) >= 0) {
		const char* valStart = current;
		int valLen = 0;
		if ((valLen = reg_pointer_star.Check(current, myNode)) >= 0) {
			myNode->value = std::string(valStart, valLen);
			parrentNode->subnodes.push_back(myNode);
			return current - start;
		}
	}

	delete myNode;
	current = (char*)start;
	return -1;
}

// Inherited via Expression
int CodeParse::Function::Check(char*& current, ParseTreeNode* parrentNode) const {
	const char* start = current;

	ParseTreeNode* myNode = new ParseTreeNode;
	myNode->type = ParseTreeNodeType_Function;

	if (pars_dataType.Check(current, myNode) >= 0) {
		if (reg_seperator_atleastOne.Check(current, myNode) >= 0) {
			if (pars_name.Check(current, myNode) >= 0) {
				if (reg_func_params.Check(current, myNode) >= 0) {
					if (reg_seperator_star.Check(current, myNode) >= 0) {
						if (pars_scope.Check(current, myNode) >= 0) {
							parrentNode->subnodes.push_back(myNode);
							return current - start;
						}
					}
				}
			}
		}
	}

	delete myNode;
	current = (char*)start;
	return -1;
}

int CodeParse::Parameter::Check(char*& current, ParseTreeNode* parrentNode) const
{
	const char* start = current;
	ParseTreeNode* myNode = new ParseTreeNode;
	myNode->type = ParseTreeNodeType_Variable;

	if (pars_dataType.Check(current, myNode) >= 0 && reg_seperator_atleastOne.Check(current, myNode) >= 0) {
		if (pars_name.Check(current, myNode) >= 0) {
			parrentNode->subnodes.push_back(myNode);
			return current - start;
		}
	}

	delete myNode;
	current = (char*)start;
	return -1;	
}

int CodeParse::Name::Check(char*& current, ParseTreeNode* parrentNode) const
{
	const char* start = current;
	ParseTreeNode* myNode = new ParseTreeNode;
	myNode->type = ParseTreeNodeType_Name;

	int i = reg_variableName_allowNameSpace_AtleastOne.Check(current, myNode);
	if (i < 0) {
		delete myNode;
		return -1;
	}

	myNode->value = std::string(start, i);
	parrentNode->subnodes.push_back(myNode);

	return i;
}

int CodeParse::ParseExpression::Check(char*& current, void* parrentNode) const
{
	return Check(current, (ParseTreeNode*)parrentNode);
}

int CodeParse::CodeCpp::Check(char*& current, ParseTreeNode* parrentNode) const
{
	return reg_code_cpp_Start_Star.Check(current, parrentNode);
}

CodeParse::Scope::Scope()
{
}

int CodeParse::Scope::Check(char*& current, ParseTreeNode* parrentNode) const
{
	const char* start = current;
	ParseTreeNode* myNode = new ParseTreeNode;
	myNode->type = ParseTreeNodeType_Scope;

	if (reg_scope_start.Check(current, myNode) >= 0){
		if (reg_scope_content_star.Check(current, myNode) >= 0 && reg_scope_end.Check(current, myNode) >= 0) {
			parrentNode->subnodes.push_back(myNode);
			return current - start;
		}
	}

	delete myNode;
	current = (char*)start;
	return -1;
}
