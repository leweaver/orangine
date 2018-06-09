#pragma once

#include "Entity.h"

#include <vector>
#include <future>

#include "Renderable_component.h"

namespace oe {
class Alpha_sorter {
public:

	struct Sort_entry {
		float distanceSqr;
		Entity* entity;
	};
			
	template<class _RanIt>
	void beginSortAsync(_RanIt begin, _RanIt end, const DirectX::SimpleMath::Vector3& eyePosition)
	{
		_entities.clear();
		for (auto iter = begin; iter != end; ++iter) {
			Entity* entity = (*iter).get();
			const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
			assert(renderable != nullptr);
			if (renderable->material()->getAlphaMode() == Material_alpha_mode::Blend)
				_entities.push_back({ 0.0f, entity });
		}
		beginSortTask(eyePosition);
	}

	void reset();
	
	void wait(const std::function<void(const std::vector<Sort_entry>&)>& callback);
	int waitTime() const { return _waitTime; }
	void setWaitTime(int waitTime) { _waitTime = waitTime; }

private:

	void beginSortTask(DirectX::SimpleMath::Vector3 eyePosition);

	std::vector<Sort_entry> _entities;
	std::future<void> _handle;
	int _waitTime = 10000;
};
}
