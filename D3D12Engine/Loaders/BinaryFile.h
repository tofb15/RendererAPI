#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <memory>

class BinaryFile {
public:
	BinaryFile();
	~BinaryFile();

	virtual bool Open(std::filesystem::path filePath) = 0;
	virtual bool Close() = 0;

protected:
	bool m_cpuIsBigEndian;
	void ReverseBytes(void* start, int size);
private:
	void SetBigEndian();
};

class BinaryFileIn : public BinaryFile {
public:
	BinaryFileIn();
	~BinaryFileIn();
	// Inherited via BinaryFile
	virtual bool Open(std::filesystem::path filePath) override;
	virtual bool Close() override;

	template<typename T>
	bool Read(T& c);
	bool ReadLine(std::string& line);

private:
	std::ifstream m_inFile;

};

class BinaryFileOut : public BinaryFile {
public:
	BinaryFileOut();
	~BinaryFileOut();
	// Inherited via BinaryFile
	virtual bool Open(std::filesystem::path filePath) override;
	virtual bool Close() override;

	template<typename T>
	bool Write(const T& c);
	bool WriteLine(const std::string& line);

private:
	std::ofstream m_outFile;

};

template<typename T>
inline bool BinaryFileIn::Read(T& c) {
	if (m_inFile.read((char*)(&c), sizeof(T))) {
		if (m_cpuIsBigEndian) {
			ReverseBytes(&c, sizeof(T));
		}
		return true;
	}
	return false;
}

template<typename T>
inline bool BinaryFileOut::Write(const T& c) {
	if (m_cpuIsBigEndian) {
		T temp = c;
		ReverseBytes(&temp, sizeof(T));
		m_outFile.write((char*)(&temp), sizeof(T));
	} else {
		m_outFile.write((char*)(&c), sizeof(T));
	}
	return true;
}
