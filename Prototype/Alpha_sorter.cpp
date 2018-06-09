#include "pch.h"
#include "Alpha_sorter.h"

#include <DirectXMath.h>
#include <future>
#include <chrono>

using namespace oe;
using namespace DirectX;

void Alpha_sorter::beginSortTask(SimpleMath::Vector3 eyePosition)
{
	_handle = std::async(std::launch::async, [this, &eyePosition]() {
		const XMVECTOR eyePosition_v = eyePosition;
		for (auto& entry : _entities) {
			entry.distanceSqr = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(eyePosition_v, entry.entity->worldPosition())));
		}

		std::sort(_entities.begin(), _entities.end(), [](const Sort_entry& lhs, const Sort_entry& rhs) {
			return lhs.distanceSqr < rhs.distanceSqr;
		}); 
	});
}

void Alpha_sorter::reset()
{
	assert(!_handle.valid());
	_entities.clear();
}

void Alpha_sorter::wait(const std::function<void(const std::vector<Sort_entry>&)>& callback)
{
	if (!_handle.valid())
		throw std::runtime_error("Must call beginSort prior to wait");

	std::future_status status;
	do {
		status = _handle.wait_for(std::chrono::milliseconds(_waitTime));
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

	_handle = std::future<void>();
}
