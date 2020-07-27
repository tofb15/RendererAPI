#include "stdafx.h"
#include "BinaryFile.h"
#include <algorithm>

BinaryFile::BinaryFile() {
	SetBigEndian();
}

BinaryFile::~BinaryFile() {
}

void BinaryFile::ReverseBytes(void* start, int size) {
	char* istart = static_cast<char*>(start);
	char* iend = istart + size;
	std::reverse(istart, iend);
}

void BinaryFile::SetBigEndian() {
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	m_cpuIsBigEndian = (bint.c[0] == 1);
}

BinaryFileIn::BinaryFileIn() : BinaryFile() {

}

BinaryFileIn::~BinaryFileIn() {
}

bool BinaryFileIn::ReadLine(std::string& line) {
	if (std::getline(m_inFile, line)) {
		return true;
	} else {
		return false;
	}
}

bool BinaryFileIn::Open(std::filesystem::path filePath) {
	if (m_inFile.is_open()) {
		Close();
	}

	m_inFile = std::ifstream(filePath, std::fstream::binary);

	return m_inFile.is_open();
}

bool BinaryFileIn::Close() {
	m_inFile.close();
	return true;
}

BinaryFileOut::BinaryFileOut() {
}

BinaryFileOut::~BinaryFileOut() {
}

bool BinaryFileOut::Open(std::filesystem::path filePath) {
	if (m_outFile.is_open()) {
		Close();
	}

	if (!std::filesystem::exists(filePath.parent_path())) {
		std::filesystem::create_directories(filePath.parent_path());
	}

	m_outFile = std::ofstream(filePath, std::fstream::binary);

	return m_outFile.is_open();
}

bool BinaryFileOut::Close() {
	m_outFile.close();
	return true;
}

bool BinaryFileOut::WriteLine(const std::string& line) {
	m_outFile << line << "\n";
	return false;
}
