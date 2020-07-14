#include "stdafx.h"
#include "SimpleTexture.h"

void LOADER::SimpleTexture::ExtractChannel(unsigned int channel, const Texture& original, Texture& outTexture) {
	outTexture.w = original.w;
	outTexture.h = original.h;
	outTexture.nChannels = 1;
	outTexture.data.clear();
	unsigned int originalSize = original.w * original.h * original.nChannels;

	for (unsigned int i = channel; i < originalSize; i += original.nChannels) {
		outTexture.data.push_back(original.data[i]);
	}
}

bool LOADER::SimpleTexture::SaveTexture(const Texture& texture, std::string file) {
	std::ofstream out(file, std::ofstream::binary);
	//std::ofstream out(file);
	if (!out.is_open()) {
		return false;
	}

	//out << texture.w << texture.h << texture.nChannels;

	out.write((char*)& texture.w, sizeof(unsigned int));
	out.write((char*)& texture.h, sizeof(unsigned int));
	out.write((char*)& texture.nChannels, sizeof(unsigned int));

	for (auto& e : texture.data) {
		out.write((char*)& e, 1);
	}
}

bool LOADER::SimpleTexture::LoadTexture(std::string file, Texture& texture) {
	return LoadTexture(file, texture.data, texture.w, texture.h, texture.nChannels);
}

bool LOADER::SimpleTexture::LoadTexture(std::string file, std::vector<unsigned char>& data, unsigned int& w, unsigned int& h, unsigned int& nChannels) {
	std::ifstream in(file, std::ofstream::binary);
	//std::ifstream in(file);
	if (!in.is_open()) {
		return false;
	}

	//in >> texture.w >> texture.h >> texture.nChannels;
	in.read((char*)& w, sizeof(unsigned int));
	in.read((char*)& h, sizeof(unsigned int));
	in.read((char*)& nChannels, sizeof(unsigned int));

	unsigned int size = w * h * nChannels;
	data.clear();
	data.reserve(size);

	unsigned char temp;
	for (int i = 0; i < size; i++) {
		in.read((char*)& temp, 1);
		data.push_back(temp);
	}
}
