#pragma once

#include "../Engine/Scene.h"

#include <functional>

class Mock_scene : public oe::Scene {
public:

	explicit Mock_scene(const std::function<void(Manager_tuple&)>& initManagersFn = [](Manager_tuple&){})
		: Scene(createManagers(initManagersFn))
	{}

private:
	Manager_tuple createManagers(const std::function<void(Manager_tuple&)>& initManagersFn)
	{
		Manager_tuple managers;

		initManagersFn(managers);

		return managers;	
	}
};
