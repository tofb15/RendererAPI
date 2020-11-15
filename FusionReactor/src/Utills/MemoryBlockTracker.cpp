#include "MemoryBlockTracker.h"
#include <iostream>
#ifdef _DEBUG
#include <intrin.h> //__debugbreak()
#endif

MemoryBlockTracker::~MemoryBlockTracker() {

#ifdef _DEBUG
	if (m_nAllocatedBlocks > 0) {
		std::cout << "==============MemoryBlockTracker================" << std::endl;
		std::cout << m_nAllocatedBlocks << " unreleased allocations found" << std::endl;
		std::cout << "Dumping Blocks... [Internal address, BlockSize, A|F(Allocated | Free)]" << std::endl;
		std::cout << ToString() << std::endl;
		std::cout << "================================================" << std::endl;
	}
#endif // _DEBUG

}

bool MemoryBlockTracker::Initialize(const size_t& size) {
	m_memList.Add(MemNode(MemmoryType::Not_Allocated, 0, size));
	m_prefferedFreeBlock = m_memList.Node_At(0);
	return true;
}

size_t MemoryBlockTracker::AllocateTracker(const size_t& size) {
	Node<MemNode>* nodeToAlloc = FindEmptyNode(size);
	if (nodeToAlloc) {
		MemNode& nodeData = nodeToAlloc->Get();

		nodeData.SetType(MemmoryType::Allocated);
		size_t sizeDiff = nodeData.GetSize() - size;
		if (sizeDiff > 0) {
			nodeData.SetSize(size);

			MemNode emptySpill(MemmoryType::Not_Allocated, nodeData, sizeDiff);
			Node<MemNode>* spillNode = m_memList.AddAfter(nodeToAlloc, emptySpill);
			m_prefferedFreeBlock = spillNode;
		} else {
			m_prefferedFreeBlock = nullptr;
		}

		m_nAllocatedBlocks++;
		return nodeData.GetStart();
	}

	return -1;
}

bool MemoryBlockTracker::ReleaseTracker(const size_t& start, const size_t& expectedSize) {
	Node<MemNode>* currentNode = m_memList.Node_At(0);
	bool wasFreed = false;
	while (currentNode) {
		MemNode& nodeData = currentNode->Get();
		if (nodeData.GetStart() == start) {
#ifdef _DEBUG
			//Only do the checks in debug
			if (expectedSize != nodeData.GetSize()) {
				//Datablock size and the expected size did not match. Was the pointer casted to another type?
				__debugbreak();
				break;
			} else if (nodeData.GetType() != MemmoryType::Allocated) {
				//Datablock was already freed.
				__debugbreak();
				break;
			}
#endif // _DEBUG

			Node<MemNode>* prev = currentNode->Prev();
			Node<MemNode>* center = currentNode;
			Node<MemNode>* next = currentNode->Next();

			MemNode& centerData = center->Get();
			centerData.SetType(MemmoryType::Not_Allocated);
			m_prefferedFreeBlock = center;

			if (next) {
				MemNode& nextData = next->Get();
				if (nextData.GetType() == MemmoryType::Not_Allocated) {
					centerData.SetSize(nextData.GetSize() + centerData.GetSize());
					//TODO: Remove next from List
					m_memList.Remove(next);
				}
			}

			if (prev) {
				MemNode& prevData = prev->Get();
				if (prevData.GetType() == MemmoryType::Not_Allocated) {
					prevData.SetSize(prevData.GetSize() + centerData.GetSize());
					//TODO: Remove Center from List
					m_memList.Remove(center);
				}
				m_prefferedFreeBlock = prev;
			}

			wasFreed = true;
			m_nAllocatedBlocks--;

			//Stop the loop
			currentNode = nullptr;
		} else {
			//Next node
			currentNode = currentNode->Next();
		}
	}

	return wasFreed;
}

std::string MemoryBlockTracker::ToString() {
	Node<MemNode>* currentNode = m_memList.Node_At(0);
	std::string s;
	while (currentNode) {
		MemNode& data = currentNode->Get();
		s += "[" + std::to_string(data.GetStart()) + ", " + std::to_string(data.GetSize()) + ", " + (data.GetType() == MemmoryType::Allocated ? "A" : "F") + "]";
		currentNode = currentNode->Next();
		if (currentNode) {
			s += "->";
		}
	}

	return s;
}

Node<MemoryBlockTracker::MemNode>* MemoryBlockTracker::FindEmptyNode(const size_t& size) {
	Node<MemNode>* bestNode = nullptr;
	Node<MemNode>* currentNode = nullptr;

	size_t sizeThreashold = size * 1.1;
	size_t nodeSize;
	bool bContinue = true;

	if (m_prefferedFreeBlock) {
		currentNode = m_prefferedFreeBlock;

		while (currentNode && bContinue) {
			MemNode& data = currentNode->Get();
			if (data.GetType() == MemmoryType::Not_Allocated && (nodeSize = data.GetSize()) >= size) {
				if (bestNode == nullptr || nodeSize < bestNode->Get().GetSize()) {
					bestNode = currentNode;
					bContinue = false;
				}
			}

			currentNode = currentNode->Next();
		}
	}

	currentNode = m_memList.Node_At(0);
	while (currentNode != m_prefferedFreeBlock && bContinue) {
		MemNode& data = currentNode->Get();
		if (data.GetType() == MemmoryType::Not_Allocated && (nodeSize = data.GetSize()) >= size) {
			if (bestNode == nullptr || nodeSize < bestNode->Get().GetSize()) {
				bestNode = currentNode;
				bContinue = false;
			}
		}

		currentNode = currentNode->Next();
	}

	return bestNode;
}

size_t MemoryBlockTracker::GetNumberOfBlocks() {
	return m_memList.Size();
}

MemoryBlockTracker::MemNode::MemNode() {
	m_type = MemmoryType::Not_Allocated;
	m_size = 0;
	m_start = 0;
}

MemoryBlockTracker::MemNode::MemNode(const MemmoryType& type, const size_t& start, const size_t& size) {
	m_type = type;
	m_start = start;
	m_size = size;
}

MemoryBlockTracker::MemNode::MemNode(const MemmoryType& type, const MemNode& prev, const size_t& size) {
	m_type = type;
	m_start = prev.m_start + prev.m_size;
	m_size = size;
}

MemoryBlockTracker::MemNode::MemNode(const MemNode& other) {
	m_type = other.m_type;
	m_start = other.m_start;
	m_size = other.m_size;
}

MemoryBlockTracker::MemmoryType MemoryBlockTracker::MemNode::GetType() const {
	return m_type;
}

size_t MemoryBlockTracker::MemNode::GetStart() const {
	return m_start;
}

size_t MemoryBlockTracker::MemNode::GetSize() const {
	return m_size;
}

void MemoryBlockTracker::MemNode::SetType(const MemmoryType& type) {
	m_type = type;
}

void MemoryBlockTracker::MemNode::SetSize(const size_t& size) {
	m_size = size;
}

MemoryBlockTracker::MemNode& MemoryBlockTracker::MemNode::operator=(const MemNode& other) {
	m_type = other.m_type;
	m_start = other.m_start;
	m_size = other.m_size;

	return *this;
}

