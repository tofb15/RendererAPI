#pragma once

//Checks if a container contains a specific element
template <template<typename> class container, typename elementType>
bool Contains(container<elementType> c, elementType e) {
	for (auto& a : c)
	{
		if (a == e) {
			return true;
		}
	}

	return false;
}