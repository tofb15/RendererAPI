#pragma once

#ifndef D3D12_TIMING_HPP
#define D3D12_TIMING_HPP
#include <vector>
#include <mutex>
#include <string>

struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;
struct ID3D12QueryHeap;
struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12Fence1;

class D3D12Timing
{
	struct Item
	{
		unsigned long long cpuStartCycle;
		unsigned long long gpuStartCycle;
		ID3D12CommandQueue* queue;
		std::vector<unsigned long long> gpuTimeStamps;
		unsigned int timeStampsCount = 0U;
		ID3D12QueryHeap* queryHeap = nullptr;
		ID3D12Resource* queryResourceCPU = nullptr;
	};
	struct CPUItem
	{
		std::vector<unsigned long long> cpuTimeStamps;
		std::string name;
	};

	D3D12Timing();
	~D3D12Timing();

public:
	static D3D12Timing* Get()
	{
		static D3D12Timing instance;
		return &instance;
	}

	void SetDevice(ID3D12Device* device);
	void InitializeCPUStartCycle();

	int InitializeNewQueue(ID3D12CommandQueue* queue);
	int InitializeNewCPUConter(const char* name);
	void AddQueueTimeStamp(int idx, ID3D12GraphicsCommandList* cmdList);
	void AddCPUTimeStamp(int idx);

private:
	bool InitializeQueryHeap(Item* item);

	static const unsigned MAX_TIME_STAMPS = 100U;

	unsigned long long m_cpuStartCycle;
	unsigned long long m_cpuFreqCycle;
	std::vector<Item> m_queues;

	std::vector<CPUItem> m_cpuTimeStamps;

	std::mutex m_mutex;
	std::mutex m_mutexCPU;

	ID3D12Device* m_device = nullptr;
};

#endif