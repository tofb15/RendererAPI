#include "../D3D12Engine/D3D12/External/LodePNG/lodepng.h"
#include "../D3D12Engine/Loaders/SimpleTexture.h"
#include "../D3D12Engine/Utills/FileSystem.h"
#include "../D3D12Engine/Utills/FileSystem.h"

#include <stdio.h>      /* printf, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <map>       
#include <sstream>       

void GenerateOneChannelTextures(const std::string& inFolder, const std::string& outFolder) {
	LOADER::SimpleTexture::Texture texture_in;
	texture_in.nChannels = 4;
	LOADER::SimpleTexture::Texture texture_out;

	FileSystem::Directory dir;
	FileSystem::ListDirectory(dir, inFolder, { ".png" });
	std::filesystem::create_directories(outFolder);

	for (auto& e : dir.files) {
		texture_in.data.clear();
		lodepng::decode(texture_in.data, texture_in.w, texture_in.h, e.path.string());
		LOADER::SimpleTexture::ExtractChannel(3, texture_in, texture_out);
		LOADER::SimpleTexture::SaveTexture(texture_out, outFolder + e.path.stem().string() + "_A" + ".simpleTexture");
	}
}

void GenerateRandomTexture(const int W, const int H) {
	LOADER::SimpleTexture::Texture texture_out;
	const int NChannels = 4;
	const int size = W * H * NChannels;
	std::string outFolder = "../../Exported_Assets/Textures/Generated/";
	std::string outName = "NoiseTexture";
	std::filesystem::create_directories(outFolder);

	texture_out.data.clear();
	texture_out.data.reserve(size);
	texture_out.w = W;
	texture_out.h = H;
	texture_out.nChannels = NChannels;

	int temp;
	for (size_t i = 0; i < size; i++) {
		temp = rand() % 256;
		texture_out.data.push_back(temp);
	}

	lodepng::encode(outFolder + outName + "_RGBA.png", texture_out.data, W, H);
	LOADER::SimpleTexture::ExtractChannel(3, texture_out, texture_out);
	LOADER::SimpleTexture::SaveTexture(texture_out, outFolder + outName + "_A" + ".simpleTexture");
}


void Data() {
	FileSystem::Directory dir;
	FileSystem::ListDirectory(dir, "C:/Users/Tobias/Desktop/Programming/Exported_Assets/TestData/Final-Remake/Tesselation/", {".MergedData"});

	std::vector<FileSystem::File> fileList = dir.GetAllFoundListedFiles();
	std::map<std::string, float> dataAverages;

	for (FileSystem::File& f : fileList) {
		std::ifstream file(f.path);
		std::vector<std::string> names;
		//std::vector<float> averages;
		int nRows = 0;

		std::string row;
		std::string column_string;
		float columnFloat;

		std::getline(file, row);
		std::stringstream ss(row);
		while (ss >> column_string) {
			names.push_back(column_string);
			dataAverages[column_string] = 0;
		}

		while (std::getline(file, row)) {
			ss = std::stringstream(row);
			int i = 0;
			nRows++;
			while (ss >> columnFloat) {
				dataAverages.at(names[i]) += columnFloat;
				i++;
			}
		}

		for (std::string& name : names) {
			dataAverages.at(name) /= nRows;
		}

		file.close();
	}

	std::ofstream outFile("C:/Users/Tobias/Desktop/Programming/Exported_Assets/TestData/Final-Remake/Tesselation/DataAverages.avg");
	for (auto& e : dataAverages) {
		outFile << e.first << "\t";
	}
	outFile << "\n";
	for (auto& e : dataAverages) {
		outFile << e.second << "\t";
	}
	outFile.close();
}

int main() {
	srand(time(NULL));

	Data();

	//GenerateOneChannelTextures("../../Exported_Assets/Textures/relevant/RGBA/", "../../Exported_Assets/Textures/relevant/A/");
	//GenerateRandomTexture(4096, 4096);

	return 0;
}