#pragma once

template <typename T>
class Node {
public:
	~Node() {};
	T& Get();
	Node<T>* Next() const;
	Node<T>* Prev() const;

private:
	template<typename T>
	friend class LinkedList;

	Node(const T& elements, Node<T>* prev, Node<T>* next);

	T m_element;
	Node<T>* m_pPrev;
	Node<T>* m_pNext;
};

template <typename T>
class LinkedList {
public:

	LinkedList() {};
	~LinkedList() {
		Clear();
	};
	void Clear() {
		if (m_pFirst) {
			Node<T>* current = m_pFirst->Next();
			while (current) {
				delete current->Prev();
				current = current->Next();
			}

			delete m_pLast;
		}

		m_pLast = m_pFirst = nullptr;
		m_nElements = m_nElementsHalf = 0;
	};
	size_t Size() {
		return m_nElements;
	};
	Node<T>* Add(const T& element, size_t index = -1);
	Node<T>* AddAfter(Node<T>* node, const T& element);

	T& At(size_t index);
	Node<T>* Node_At(size_t index);

	Node<T>* Remove(size_t index);
	Node<T>* Remove(Node<T>* node);

private:
	size_t m_nElements = 0;
	size_t m_nElementsHalf = 0;
	Node<T>* m_pFirst = nullptr;
	Node<T>* m_pLast = nullptr;
};

template<typename T>
Node<T>* LinkedList<T>::Add(const T& element, size_t at) {
	Node<T>* newNode = nullptr;
	if (m_nElements == 0) {
		m_pFirst = m_pLast = newNode = new Node<T>(element, nullptr, nullptr);
	} else {
		if (at >= m_nElements) {
			newNode = new Node<T>(element, m_pLast, nullptr);
			m_pLast->m_pNext = newNode;
			m_pLast = newNode;
		} else if (at == 0) {
			newNode = new Node<T>(element, nullptr, m_pFirst);
			m_pFirst->m_pPrev = newNode;
			m_pFirst = newNode;

		} else {
			Node<T>* next = Node_At(at);
			Node<T>* prev = next->Prev();
			newNode = new Node<T>(element, prev, next);
			prev->m_pNext = newNode;
			next->m_pPrev = newNode;
		}
	}
	m_nElements++;
	m_nElementsHalf = m_nElements >> 1;
	return newNode;
}

template<typename T>
Node<T>* LinkedList<T>::AddAfter(Node<T>* node, const T& element) {
	Node<T>* next = node->m_pNext;
	Node<T>* nodeNew = new Node<T>(element, node, next);
	node->m_pNext = nodeNew;

	if (next) {
		next->m_pPrev = nodeNew;
	} else {
		m_pLast = nodeNew;
	}

	m_nElements++;
	m_nElementsHalf = m_nElements >> 1;
	return nodeNew;
}

template<typename T>
inline T& LinkedList<T>::At(size_t index) {
	Node<T>* currentNode = Node_At(index);
	return currentNode->Get();
}

template<typename T>
inline Node<T>* LinkedList<T>::Node_At(size_t index) {
	if (index >= m_nElements) {
		return nullptr;
	}

	size_t currindex = 0;
	size_t steps = index;
	Node<T>* currentNode = nullptr;
	if (index > m_nElementsHalf) {
		currindex = m_nElements - 1;
		currentNode = m_pLast;
		steps = (m_nElements - 1) - index;

		for (size_t i = 0; i < steps; i++) {
			currentNode = currentNode->Prev();
		}

	} else {
		currentNode = m_pFirst;
		for (size_t i = 0; i < steps; i++) {
			currentNode = currentNode->Next();
		}
	}

	return currentNode;
}
template<typename T>
inline Node<T>* LinkedList<T>::Remove(size_t index) {
	if (index >= m_nElements) {
		return nullptr;
	}

	Node<T>* toRemove = Node_At(index);
	return Remove(toRemove);
}

template<typename T>
inline Node<T>* LinkedList<T>::Remove(Node<T>* node) {
	if (!node) {
		return nullptr;
	}

	Node<T>* next = node->Next();
	Node<T>* prev = node->Prev();

	delete node;

	if (prev) {
		prev->m_pNext = next;
	} else {
		m_pFirst = next;
	}

	if (next) {
		next->m_pPrev = prev;
	} else {
		m_pLast = prev;
	}

	m_nElements--;
	m_nElementsHalf = m_nElements >> 1;
}

template<typename T>
inline T& Node<T>::Get() {
	return m_element;
}

template<typename T>
inline  Node<T>* Node<T>::Next() const {
	return m_pNext;
}

template<typename T>
inline  Node<T>* Node<T>::Prev() const {
	return m_pPrev;
}

template<typename T>
inline  Node<T>::Node(const T& element, Node* prev, Node* next) {
	m_element = element;
	m_pPrev = prev;
	m_pNext = next;
}