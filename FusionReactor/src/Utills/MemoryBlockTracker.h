#pragma once
#include "LinkedList.h"
#include <string>

class MemoryBlockTracker {
public:
	enum class MemmoryType {
		Not_Allocated,
		Allocated,
	};

	class MemNode {
	public:
		MemNode();
		MemNode(const MemmoryType& type, const size_t& start, const size_t& size);
		MemNode(const MemmoryType& type, const MemNode& prev, const size_t& size);
		MemNode(const MemNode& other);

		~MemNode() {};

		MemmoryType GetType() const;
		size_t GetStart() const;
		size_t GetSize() const;

		void SetType(const MemmoryType& type);
		void SetSize(const size_t& size);

		MemNode& operator=(const MemNode& other);

	protected:
		MemmoryType m_type;
		size_t m_start = 0;
		size_t m_size = 0;
	};
	MemoryBlockTracker() {};
	virtual ~MemoryBlockTracker();

	virtual bool Initialize(const size_t& size);
	size_t AllocateTracker(const size_t& size);
	bool ReleaseTracker(const size_t& start, const size_t& expectedSize);
	std::string	ToString();

	Node<MemNode>* FindEmptyNode(const size_t& size);

	size_t GetNumberOfBlocks();
private:
	LinkedList<MemNode> m_memList;
	Node<MemNode>* m_prefferedFreeBlock = nullptr;
	size_t m_nAllocatedBlocks = 0;
};
