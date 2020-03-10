#include "../D3D12Engine/Utills/FileSystem.h"
#include "../D3D12Engine/Utills/RegularExpressions.h"
#include "../D3D12Engine/Utills/CodeParser.h"

#include <iostream>
#include <fstream>
#include <string>
//0
bool RemoveMultilineComment(std::string& line, std::ifstream& in) {
	char nextChar;
	int i;
	while (std::getline(in, line))
	{
		i = line.find_first_of("*");

		if (i >= 0) {
			nextChar = line[i + 1];
			if (nextChar == '/') {
				line = line.substr(i + 2, line.length() - (i + 2));
				return true;
			}
		}
	}

	return true;
}

bool ParseShader(std::filesystem::path file) {
	std::ifstream in(file);
	std::ofstream tempOut(file.string() + ".temp");

	if (!in.is_open()) {
		return false;
	}

	std::string line;
	std::string line2;
	std::string prevLine;
	char nextChar;
	char tempChar2 = 'a';
	int i;
	int longest = 0;
	int lineNr = 0;
	while (std::getline(in, line))
	{
		lineNr++;
		if (lineNr == 3338) {
			tempChar2 = tempChar2;
		}
		i = line.find_first_of("/");
		if (line.length() > longest) {
			longest = line.length();
			line2 = line;
		}
		if (i >= 0) {
			nextChar = line[i + 1];
			if (nextChar == '/') {
				line = line.substr(0, i);
			}else if (nextChar == '*') {
				bool oneRow = false;
				for (size_t j = i+2; j < line.length()-1 && !oneRow; j++)
				{
					if (line[j] == '*' && line[j+1] == '/') {
						oneRow = true;
						line = line.substr(i, j + 1 - i);
					}
				}

				if (!oneRow) {
					line = line.substr(0, i);
					if (line != "" && prevLine != "") {
						tempOut << line << "\n";
						prevLine = line;
					}

					RemoveMultilineComment(line, in);
				}
			}
		}

		if (line != "" || prevLine != "") {
			tempOut << line << "\n";
		}
		prevLine = line;

	}

	return true;
}

void ReadSource(std::string& s) {
	std::ifstream t("Source.cpp");
	if (!t.is_open()) {
		return;
	}

	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	s.resize(size);
	t.seekg(0);
	t.read(&s[0], size);
}

void printDepth(int depth) {
	for (size_t i = 0; i < depth; i++)
	{
		std::cout << "   ";
	}
}

void printTree(CodeParse::ParseTreeNode* root, int depth = 0) {
	int childSkip = 0;
	switch (root->type)
	{
	//case CodeParse::ParseTreeNodeType_Root:
	//	printDepth(depth);
	//	std::cout << "Root" << std::endl;
	//	break;
	//case CodeParse::ParseTreeNodeType_Function:
	//	printDepth(depth);
	//	std::cout << "Function - ret: " << root->subnodes[0]->subnodes[0]->value << root->subnodes[0]->value;
	//	std::cout << " name: " << root->subnodes[1]->value << std::endl;
	//	childSkip = 2;
	//	break;
	//case CodeParse::ParseTreeNodeType_Variable:
	//	break;
	//case CodeParse::ParseTreeNodeType_DataType:
	//	break;
	//case CodeParse::ParseTreeNodeType_Scope:
	//	break;
	default:
		printDepth(depth);
		std::cout << CodeParse::ParseTreeNodeType_string[root->type] << " - " << root->value << std::endl;
		break;
	}

	int n = 0;
	for (auto& e : root->subnodes)
	{
		if (n++ < childSkip) { continue; }
		printTree(e, depth + 1);
	}
}

void RegTest() {
	//std::string text = "int MinFunc(test* a, int&&&&* b2, hej hej)";
	std::string text = "#include \"test.h\"";
	ReadSource(text);

	char* start = &text.front();
	char* end = &text.back() + 1;

	RegularExp::Any a;
	RegularExp::Star s(a);
	RegularExp::Term term("0b100110123");

	CodeParse::Function func;
	CodeParse::ParseTreeNode root = {CodeParse::ParseTreeNodeType_Root};
	int i = CodeParse::pars_code_cpp.Check(start, &root);
	std::string parsedCode = text.substr(0, i) + "\0";
	if (i > 0) {
		std::cout << parsedCode;
	}
	std::cout << "======================" << std::endl;
	std::cout << "======================" << std::endl;
	printTree(&root);

	i = 0;
}

int main() {

	RegTest();

	//FileSystem::Directory dir;
	//FileSystem::ListDirectory(dir, "../../Exported_Assets/Shader/ParseTest/", {".hlsl"});
	//
	//std::ifstream in;
	//for (auto& e : dir.files)
	//{
	//	std::cout << e.path.string() << std::endl;
	//	ParseShader(e.path);
	//}

	return 0;
}