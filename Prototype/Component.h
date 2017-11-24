#pragma once

#include <string>

namespace OE {

/**
 * Abstract base class for components, which are added to entities.
 */
class Component
{
protected:
	const std::string m_name;

public:

	Component(const std::string& name) 
		: m_name(name)
	{		
	}

	virtual ~Component() {}
	
	virtual void Init() = 0;
	virtual void Update() = 0;
	
	const std::string& Name() const
	{
		return m_name;
	}

};

}

#define DECLARE_COMPONENT(name) class name : public Component {\
	private:
