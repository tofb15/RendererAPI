#pragma once
#include <string>

constexpr unsigned int m_N_DATA_POINTS = 100;

class Profiler
{
public:
	Profiler();
	~Profiler();

	void SetTitle(std::string title);
	void SetUpdateRate(float updateRate);
	void Render();
	void AddData(float data);

private:
	std::string m_title = "noTitle";
	float m_data[m_N_DATA_POINTS];
	float m_nextDataSum;

	int m_maxIndex;

	unsigned int m_nextDataNSamples;

	unsigned int m_dataCounter;
	unsigned int m_updateRate;
};

