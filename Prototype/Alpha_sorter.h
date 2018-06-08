#pragma once

#include "Entity.h"

#include <vector>
#include <future>

namespace oe {
class Alpha_sorter {
public:

	struct Sort_entry {
		float distanceSqr;

		// TODO: Don't store smart pointers (they are slow to copy on assignment).
		std::shared_ptr<Entity> entity;
	};
			
	template<class _RanIt>
	void updateEntityList(_RanIt begin, _RanIt end)
	{
		_entities.clear();
		for (auto iter = begin; iter != end; ++iter) {
			_entities.push_back({ 0.0f, *iter });
		}
	}

	void beginSort(DirectX::SimpleMath::Vector3 eyePosition);

	void wait(const std::function<void(const std::vector<Sort_entry>&)>& callback);
	int waitTime() const { return _waitTime; }
	void setWaitTime(int waitTime) { _waitTime = waitTime; }

private:
	std::vector<Sort_entry> _entities;
	std::future<void> _handle;
	int _waitTime = 10000;
};
}
