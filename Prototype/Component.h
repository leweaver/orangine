#pragma once

namespace OE {

/**
 * Abstract base class for components, which are added to entities.
 */
class Component
{

public:

	Component() 
	{		
	}

	virtual ~Component() {}
	
	virtual void Init() = 0;
	virtual void Update() = 0;

};

}

#define DECLARE_COMPONENT(name) class name : public Component {\
	private:
