#include "Profiler.h"
#include "../D3D12Engine/External/IMGUI/imgui.h"


Profiler::Profiler()
{
	m_dataCounter = 0;
	m_updateRate = 1;
	m_maxIndex = 0;

}


Profiler::~Profiler()
{
}

void Profiler::SetTitle(std::string title)
{
	m_title = title;
}

void Profiler::SetUpdateRate(float updateRate)
{
	m_updateRate = updateRate;
}

void Profiler::Render()
{
	ImGui::PlotLines(m_title.c_str(), m_data, 100, 0, "", 0, 1000, ImVec2(0, 100));
}

void Profiler::AddData(float data)
{
	m_nextDataSum += data;
	m_nextDataNSamples++;
	m_data[m_dataCounter] = m_nextDataSum / m_nextDataNSamples;

	//if (data > m_data[m_maxIndex]) {
	//	m_maxIndex = m_dataCounter;
	//}else if (m_maxIndex == m_dataCounter) {
	//	for (size_t i = 0; i < m_N_DATA_POINTS; i++)
	//	{
	//		if (m_maxIndex == i) {
	//			continue;
	//		}
	//		if (m_data[i] > m_data[m_maxIndex]) {
	//			m_maxIndex = i;
	//		}
	//	}
	//}
		
	++m_dataCounter %= 100;
}
