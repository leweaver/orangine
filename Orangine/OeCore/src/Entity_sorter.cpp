﻿#include "pch.h"
#include "OeCore/Entity_sorter.h"
#include "OeCore/EngineUtils.h"

using namespace oe;
using namespace DirectX;

void Entity_alpha_sorter::sortEntities(const SSE::Vector3& eyePosition)
{
	//const XMVECTOR eyePosition_v = eyePosition;
	for (auto& entry : _entities) {
		//entry.distanceSqr = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(eyePosition_v, entry.entity->worldPosition())));
		entry.distanceSqr = SSE::lengthSqr(eyePosition - toVector3(entry.entity->worldPosition()));
	}

	std::sort(_entities.begin(), _entities.end(), [](const Entity_alpha_sorter_entry& lhs, const Entity_alpha_sorter_entry& rhs) {
		return lhs.distanceSqr < rhs.distanceSqr;
	});
}
