#pragma once
#include "Entity.h"
#include <vector>
#include <set>
#include <map>

namespace OE {

class EntityManager
{
	friend Entity;

	std::map<unsigned int, std::unique_ptr<Entity>> m_rootEntities;
	
	// Values as of the last call to Tick.
	float m_deltaTime = 0;
	float m_elapsedTime = 0;
	unsigned int lastEntityId = 0;

public:
	const float& MElapsedTime() const
	{
		return m_elapsedTime;
	}

	const float& MDeltaTime() const
	{
		return m_deltaTime;
	}

	void Tick(float elapsedTime);

	Entity& Instantiate(std::string name);
	Entity& Instantiate(std::string name, Entity& parent);
	
private:
	std::unique_ptr<Entity> TakeRoot(Entity& entity);
	void PutRoot(std::unique_ptr<Entity>& entity);
};

}
