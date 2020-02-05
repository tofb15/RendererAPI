#include "stdafx.h"
#include "LightSource.h"

LightSource::LightSource()
{
}

LightSource::~LightSource()
{
}

void LightSource::setPosition(const Float3& pos)
{
	m_position = pos;
}

void LightSource::setColor(const Float3& color)
{
	m_color = color;
}

Float3 LightSource::getPosition() const
{
	return m_position;
}

Float3 LightSource::getColor() const
{
	return m_color;
}
