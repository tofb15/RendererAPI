#pragma once
#include "Math.hpp"

class LightSource {
public:
	LightSource();
	~LightSource();

	Float3 m_position_animated; //Current position
	Float3 m_position_center;   //Fixed Center position from where the light is anchored.
	Float3 m_color = Float3(1, 1, 1);				//Color of the light
	float m_reachRadius = 160.0;
	bool m_enabled = true;
};