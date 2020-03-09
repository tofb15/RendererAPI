#include "../D3D12Engine/Utills/FileSystem.h"
#include "../D3D12Engine/Utills/RegularExpressions.h"

#include <iostream>
#include <fstream>
#include <string>

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

void RegTest() {
	std::string text = "1203abc";
	char* start = &text.front();
	char* end = &text.back();

	RegularExp::DigitNonZero d;
	int i = d.Check(start, end);
	if (i > 0) {
		std::cout << text.substr(0, i);
	}

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