#pragma once

#include "OeCore/Entity.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Collision.h"

#include <vector>
#include <future>
#include <chrono>

namespace oe {
	template <class TEntity_data>
	class Entity_sorter {
	public:

		// Reset the state of the sorter
		void reset();

		// Block until the sorter has completed, then call the given function with the sorted entities.
		// Subsequent calls to this method will resolve instantly with the same entity list, until
		// reset() is called.
		void waitThen(const std::function<void(const std::vector<TEntity_data>&)>& callback);
		
		int timeoutMs() const { return _timeoutMs; }
		void setTimeoutMs(int waitTime) { _timeoutMs = waitTime; }

	protected:
		std::vector<TEntity_data> _entities;
		std::future<void> _handle;
		int _timeoutMs = 10000;

	};

	struct Entity_cull_sorter_entry {
		Entity* entity;
	};

	class Entity_cull_sorter : public Entity_sorter<Entity_cull_sorter_entry> {
	public:

		template<class _RanIt>
		void beginSortAsync(_RanIt begin, _RanIt end, const BoundingFrustumRH& cullingFrustum)
		{
			_entities.clear();

			_handle = std::async(std::launch::async, [this, cullingFrustum, begin, end]() {

				for (auto iter = begin; iter != end; ++iter) {
					Entity* entity = (*iter).get();

					if (cullingFrustum.Contains(entity->boundSphere()))
						_entities.push_back({ entity });
				}
			});
		}
	};

	struct Entity_alpha_sorter_entry {
		float distanceSqr;
		Entity* entity;
	};

	class Entity_alpha_sorter : public Entity_sorter<Entity_alpha_sorter_entry> {
	public:

		template<class _RanIt>
		void beginSortAsync(_RanIt begin, _RanIt end, const SSE::Vector3& eyePosition)
		{
			using namespace DirectX;

			_handle = std::async(std::launch::async, [this, &eyePosition, begin, end]() 
			{
				_entities.clear();
				for (auto iter = begin; iter != end; ++iter) {
					Entity* entity = iter->entity;
					const auto renderable = entity->getFirstComponentOfType<Renderable_component>();
					assert(renderable != nullptr);

					const auto& material = renderable->material();
					if (material != nullptr && material->getAlphaMode() == Material_alpha_mode::Blend)
						_entities.push_back({ 0.0f, entity });
				}

				sortEntities(eyePosition);			
			});
		}

	protected:
		void sortEntities(const SSE::Vector3& eyePosition);
	};

	template <class TEntity_data>
	void Entity_sorter<TEntity_data>::reset()
	{
		assert(!_handle.valid());
		_entities.clear();
	}

	template <class TEntity_data>
	void Entity_sorter<TEntity_data>::waitThen(const std::function<void(const std::vector<TEntity_data>&)>& callback)
	{
		if (!_handle.valid())
			throw std::runtime_error("Must call beginSort prior to wait");

		std::future_status status;
		do {
			status = _handle.wait_for(std::chrono::milliseconds(_timeoutMs));
			if (status == std::future_status::ready) {
				callback(_entities);
			}
			else if (status == std::future_status::timeout) {
				LOG(WARNING) << "Alpha sort exceeded maximum wait time";
			}
			else {
				throw std::runtime_error("Unexpected future_status in Alpha_sort::wait");
			}
		} while (status != std::future_status::ready);
	}
}
