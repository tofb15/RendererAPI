#pragma once
#include <fstream>
#include <vector>

namespace LOADER {
	namespace SimpleTexture {
		struct Texture {
			std::vector<unsigned char> data;
			unsigned int w;
			unsigned int h;
			unsigned int nChannels;
		};

		void ExtractChannel(unsigned int channel, const Texture& original, Texture& outTexture);
		bool SaveTexture(const Texture& texture, std::string file);
		bool LoadTexture(std::string file, Texture& texture);
		bool LoadTexture(std::string file, std::vector<unsigned char>& data, unsigned int& w, unsigned int& h, unsigned int& nChannels);
	}
}