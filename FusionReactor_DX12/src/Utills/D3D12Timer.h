#pragma once

#include <d3d12.h>

namespace FR
{
	struct GPUTimestampPair
	{
		UINT64 Start;
		UINT64 Stop;
	};

	// D3D12 timer.
	class D3D12Timer {
	public:
		// Constructor.
		D3D12Timer();

		// Destructor.
		~D3D12Timer();

		HRESULT Init(ID3D12Device* pDevice, UINT numTimers);

		// Start timestamp.
		void Start(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex);

		// Stop timestamp.
		void Stop(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex);

		// Resolve query data. Write query to device memory. Make sure to wait for query to finsih before resolving data.
		void ResolveQueryToCPU(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndex);
		void ResolveQueryToCPU(ID3D12GraphicsCommandList* pCommandList, UINT timestampPairIndexFirst, UINT timestampPairIndexLast);
		void ResolveQueryToGPU(ID3D12GraphicsCommandList* pCommandList, ID3D12Resource** ppQueryResourceGPUOut);

		GPUTimestampPair GetTimestampPair(UINT timestampPairIndex);

		// Calcluate time and map memory to CPU.
		void CalculateTime();

		// Get time from start to stop in nano seconds.
		UINT64 GetDeltaTime();
		UINT64 GetEndTime();
		UINT64 GetBeginTime();

		// Whether timer is active.
		bool IsActive();

	private:
		void SetGPUResourceState(ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES prevState, D3D12_RESOURCE_STATES newState);

		ID3D12Device* device_ = nullptr;
		ID3D12QueryHeap* queryHeap_ = nullptr;
		ID3D12Resource* queryResourceCPU_ = nullptr;
		ID3D12Resource* queryResourceGPU_ = nullptr;
		bool active_ = false;
		UINT64 deltaTime_ = 0;
		UINT64 beginTime_ = 0;
		UINT64 endTime_ = 0;
		UINT timerCount_ = 0;
	};
}