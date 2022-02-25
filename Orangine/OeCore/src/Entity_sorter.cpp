#include "OeCore/Entity_sorter.h"

using namespace oe;
using namespace DirectX;

void Entity_alpha_sorter::sortEntities(const SSE::Vector3& eyePosition)
{
	for (auto& entry : _entities) {
		entry.distanceSqr = SSE::lengthSqr(eyePosition - entry.entity->worldPosition());
	}

	std::sort(_entities.begin(), _entities.end(), [](const Entity_alpha_sorter_entry& lhs, const Entity_alpha_sorter_entry& rhs) {
		return lhs.distanceSqr < rhs.distanceSqr;
	});
}
