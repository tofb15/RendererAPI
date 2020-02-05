#pragma once
#include "Math.hpp"

class LightSource
{
public:
	LightSource();
	~LightSource();

	void setPosition(const Float3& pos);
	void setColor(const Float3& color);
	Float3 getPosition() const;
	Float3 getColor() const;

private:
	Float3 m_position;
	Float3 m_color;
};