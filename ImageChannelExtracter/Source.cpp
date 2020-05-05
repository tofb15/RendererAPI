#include "../D3D12Engine/D3D12/External/LodePNG/lodepng.h"
#include "../D3D12Engine/Loaders/SimpleTexture.h"
#include "../D3D12Engine/Utills/FileSystem.h"

#include <stdio.h>      /* printf, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

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

int main() {
	srand(time(NULL));

	//GenerateOneChannelTextures("../../Exported_Assets/Textures/relevant/RGBA/", "../../Exported_Assets/Textures/relevant/A/");
	GenerateRandomTexture(4096, 4096);

	return 0;
}