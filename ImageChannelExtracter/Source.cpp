#include "../D3D12Engine/D3D12/External/LodePNG/lodepng.h"
#include "../D3D12Engine/Loaders/SimpleTexture.h"
#include "../D3D12Engine/Utills/FileSystem.h"

int main() {
	LOADER::SimpleTexture::Texture texture_in;
	texture_in.nChannels = 4;
	LOADER::SimpleTexture::Texture texture_out;

	FileSystem::Directory dir;
	FileSystem::ListDirectory(dir, "../../Exported_Assets/Textures/relevant/RGBA", { ".png" });
	std::filesystem::create_directories("../../Exported_Assets/Textures/relevant/A/");

	for (auto& e : dir.files) {
		texture_in.data.clear();
		lodepng::decode(texture_in.data, texture_in.w, texture_in.h, e.path.string());
		LOADER::SimpleTexture::ExtractChannel(3, texture_in, texture_out);
		LOADER::SimpleTexture::SaveTexture(texture_out, "../../Exported_Assets/Textures/relevant/A/" + e.path.stem().string() + "_A" + ".simpleTexture");
	}

	return 0;
}