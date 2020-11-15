#pragma once
#include "LinkedList.h"
#include <string>
#include "MemoryBlockTracker.h"
#ifdef _DEBUG
#include <intrin.h> //__debugbreak()
#endif

class MemoryManagerRAM : public MemoryBlockTracker {
public:
	MemoryManagerRAM() {};
	~MemoryManagerRAM() {
		if (m_dataBlock) {
			delete[] m_dataBlock;
		}
	};

	bool Initialize(size_t size);
	template<typename T>
	T* New();
	template<typename T>
	void New(T*& pointer);

	template<typename T>
	void Free(T* pointer);

private:
	unsigned char* m_dataBlock = nullptr;
	size_t m_size = 0;
};

template<typename T>
inline T* MemoryManagerRAM::New() {
	size_t start = AllocateTracker(sizeof(T));

	if (start == -1) {
#ifdef _DEBUG
		__debugbreak();//Memmory Pool ran out of free memmory. Consider increesing its size.
#endif // _DEBUG
		return nullptr;
	}

	memset(&m_dataBlock[start], 0, sizeof(T));
	T* element = reinterpret_cast<T*>(&m_dataBlock[start]);

	return element;
}
template<typename T>
inline void MemoryManagerRAM::New(T*& pointer) {
	size_t start = AllocateTracker(sizeof(T));

	if (start == -1) {
#ifdef _DEBUG
		__debugbreak();//Memmory Pool ran out of free memmory. Consider increesing its size.
#endif // _DEBUG
		pointer = nullptr;
	}

	memset(&m_dataBlock[start], 0, sizeof(T));
	pointer = reinterpret_cast<T*>(&m_dataBlock[start]);
}

bool MemoryManagerRAM::Initialize(size_t size) {
	MemoryBlockTracker::Initialize(size);
	m_size = size;
	m_dataBlock = new unsigned char[size];
	return true;
}

template<typename T>
inline void MemoryManagerRAM::Free(T* pointer) {
	size_t start = (size_t)((unsigned char*)pointer - m_dataBlock);
	if (start < m_size) {
		if (ReleaseTracker(start, sizeof(T))) {
			pointer->~T();
		} else {
			//Error
#ifdef _DEBUG
			__debugbreak();//Data pointed to by "pointer" was not allocated by this memmory manager!
#endif // _DEBUG
		}
	}
}
