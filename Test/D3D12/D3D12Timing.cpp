#include "D3D12Timing.hpp"
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <fstream>

D3D12Timing::D3D12Timing()
{
	m_queues.reserve(100);
	m_cpuTimeStamps.reserve(100);
}

D3D12Timing::~D3D12Timing()
{
	ID3D12Fence1*				fence = nullptr;
	void*						eventHandle = nullptr;
	unsigned long long			fenceValue = 0ULL;

	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	fenceValue = 1;
	eventHandle = CreateEvent(0, false, false, 0);


	for (int i = 0; i < m_queues.size(); i++)
	{
		Item& item = m_queues[i];

		// Wait until execution finishes

		const UINT64 fenceV = fenceValue++;
		item.queue->Signal(fence, fenceV);

		if (fence->GetCompletedValue() < fenceV)
		{
			fence->SetEventOnCompletion(fenceV, eventHandle);
			WaitForSingleObject(eventHandle, 10000);
		}


		unsigned int n_values = min(MAX_TIME_STAMPS, m_queues[i].timeStampsCount);

		unsigned long long* mapMem = nullptr;
		D3D12_RANGE readRange{ 0, sizeof(UINT64) * n_values };
		D3D12_RANGE writeRange{ 0, 0 };
		if (SUCCEEDED(item.queryResourceCPU->Map(0, &readRange, (void**)&mapMem)))
		{
			for (int j = 0; j < n_values; j++)
			{
				item.gpuTimeStamps.push_back(*mapMem);
				mapMem++;
			}
			item.queryResourceCPU->Unmap(0, &writeRange);
		}


		// Time calculations
		unsigned long long queueFreq;
		item.queue->GetTimestampFrequency(&queueFreq);

		long long creationTimeMys = (item.cpuStartCycle - m_cpuStartCycle) * (1'000'000.0 / m_cpuFreqCycle);


		std::ofstream ostr(std::string("TimeStamps") + std::to_string(i) + ".tsv");
		if (ostr.is_open())
		{
			D3D12_COMMAND_LIST_TYPE type = item.queue->GetDesc().Type;
			ostr << "#Index: " << i << ", Type: " <<
				(type == D3D12_COMMAND_LIST_TYPE_DIRECT ? "Direct" :
				(type == D3D12_COMMAND_LIST_TYPE_COPY ? "Copy" : "Compute"))
				<< "\n";

			int c = 0;
			for (int j = 0; j < item.gpuTimeStamps.size(); j+=2)
			{
				long long beganExecution = (item.gpuTimeStamps[j] - item.gpuStartCycle) * (1'000'000.0 / queueFreq);
				beganExecution += creationTimeMys;

				long long finishExecution = (item.gpuTimeStamps[j + 1] - item.gpuStartCycle) * (1'000'000.0 / queueFreq);
				finishExecution += creationTimeMys;

				ostr << c << "\t" << beganExecution << "\n" << c << "\t" << finishExecution << "\n\n";
				c++;
			}

			ostr.close();
		}

		item.queryResourceCPU->Release();
		item.queryHeap->Release();
		item.queue->Release();
	}

	double cpuCycleToMys = (1'000'000.0 / m_cpuFreqCycle);

	for (int i = 0; i < m_cpuTimeStamps.size(); i++)
	{
		CPUItem* item = &m_cpuTimeStamps[i];

		std::ofstream ostr(std::string("TimeStampsCPU") + std::to_string(i) + ".tsv");

		if (ostr.is_open())
		{
			ostr << "#Index: " << i << ", Name: " << item->name << "\n";
			
			int c = 0;

			for (int j = 0; j < item->cpuTimeStamps.size(); j += 2)
			{
				long long began = (item->cpuTimeStamps[j] - m_cpuStartCycle) * cpuCycleToMys;
				long long end = (item->cpuTimeStamps[j + 1] - m_cpuStartCycle) * cpuCycleToMys;

				ostr << c << "\t" << began << "\n" << c << "\t" << end << "\n\n";
				c++;
			}

			ostr.close();
		}
	}

	m_device->Release();
	fence->Release();
}

void D3D12Timing::SetDevice(ID3D12Device * device)
{
	m_device = device;
}

bool D3D12Timing::InitializeQueryHeap(Item* item)
{
	HRESULT hr;

	D3D12_QUERY_HEAP_DESC queryHeapDesc;
	queryHeapDesc.NodeMask = 0;
	queryHeapDesc.Count = MAX_TIME_STAMPS;
	
	switch (item->queue->GetDesc().Type)
	{
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP;
		break;
	default:
		return false;
	}

	hr = m_device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&item->queryHeap));
	if (FAILED(hr))
	{
		return false;
	}

	D3D12_RESOURCE_DESC resouceDesc;
	ZeroMemory(&resouceDesc, sizeof(resouceDesc));
	resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resouceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resouceDesc.Width = sizeof(unsigned long long) * MAX_TIME_STAMPS;
	resouceDesc.Height = 1;
	resouceDesc.DepthOrArraySize = 1;
	resouceDesc.MipLevels = 1;
	resouceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resouceDesc.SampleDesc.Count = 1;
	resouceDesc.SampleDesc.Quality = 0;
	resouceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resouceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_READBACK;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resouceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&item->queryResourceCPU)
	);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

void D3D12Timing::InitializeCPUStartCycle()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	m_cpuStartCycle = li.QuadPart;

	QueryPerformanceFrequency(&li);
	m_cpuFreqCycle = li.QuadPart;
}

int D3D12Timing::InitializeNewQueue(ID3D12CommandQueue * queue)
{
	queue->AddRef();

	Item item;
	item.queue = queue;
	
	queue->GetClockCalibration(&item.gpuStartCycle, &item.cpuStartCycle);

	InitializeQueryHeap(&item);

	int idx;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queues.push_back(item);
		idx = m_queues.size() - 1;
	}

	// Item is local, but queryResource is a pointer
	std::wstring wstr = L"queryResource " + std::to_wstring(idx);
	item.queryResourceCPU->SetName(wstr.c_str());

	return idx;
}

int D3D12Timing::InitializeNewCPUConter(const char* name)
{
	CPUItem item;
	item.name = name;

	int idx;
	{
		std::lock_guard<std::mutex> lock(m_mutexCPU);
		m_cpuTimeStamps.push_back(item);
		idx = m_cpuTimeStamps.size() - 1;
	}
	
	return idx;
}

void D3D12Timing::AddQueueTimeStamp(int idx, ID3D12GraphicsCommandList * cmdList)
{
	Item* item;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		item = &m_queues[idx];
	}

	unsigned int queryHeapIndex = item->timeStampsCount % MAX_TIME_STAMPS;

	if (item->timeStampsCount >= MAX_TIME_STAMPS)
	{
		unsigned long long* mapMem = nullptr;
		D3D12_RANGE readRange{ sizeof(UINT64) * queryHeapIndex, sizeof(UINT64) * (queryHeapIndex + 1) };
		D3D12_RANGE writeRange{ 0, 0 };
		if (SUCCEEDED(item->queryResourceCPU->Map(0, &readRange, (void**)&mapMem)))
		{
			mapMem += queryHeapIndex;
			item->gpuTimeStamps.push_back(*mapMem);
			item->queryResourceCPU->Unmap(0, &writeRange);
		}
	}

	cmdList->EndQuery(item->queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, queryHeapIndex);
	cmdList->ResolveQueryData(
		item->queryHeap,
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryHeapIndex,
		1,
		item->queryResourceCPU,
		sizeof(unsigned long long) * queryHeapIndex
	);

	item->timeStampsCount++;
}

void D3D12Timing::AddCPUTimeStamp(int idx)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	{
		std::lock_guard<std::mutex> lock(m_mutexCPU);
		m_cpuTimeStamps[idx].cpuTimeStamps.push_back(li.QuadPart);
	}
}