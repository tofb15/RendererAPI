#pragma once
#include "Math.hpp"

class LightSource
{
public:
	LightSource();
	~LightSource();

	Float3 m_position_animated;
	Float3 m_position_center;
	Float3 m_color;
};