#pragma once
class ParticleSystem
{
public:
	ParticleSystem();
	virtual ~ParticleSystem();

	virtual void Update(float dt) = 0;
protected:
	unsigned short m_id;
};

