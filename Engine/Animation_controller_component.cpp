#include "pch.h"
#include "Animation_controller_component.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Animation_controller_component);

void Animation_controller_component::addAnimation(std::string animationName, std::unique_ptr<Animation> animation)
{
    if (animationName.empty())
        throw std::logic_error("Animation must have a name.");
    if (containsAnimationByName(animationName))
        throw std::logic_error("Animation already exists with name: " + animationName);

    for (const auto& channel : animation->channels) {
        if (channel->keyframeTimes->empty())
            throw std::runtime_error("Animation channel must have at least 1 keyframe");

        if (channel->interpolationType == Animation_interpolation::Cubic_Spline) {
            if (channel->keyframeTimes->size() < 2)
                throw std::runtime_error("Cubic spline animation channels must have at least 2 keyframes");
        }

        if (static_cast<uint32_t>(channel->keyframeTimes->size()) * channel->valuesPerKeyFrame != channel->keyframeValues->count) {
            throw std::runtime_error("Expected " + std::to_string(channel->valuesPerKeyFrame) + 
                " values per keyframe. Instead had: " + std::to_string(channel->keyframeValues->count / channel->keyframeTimes->size()));
        }

        // TODO: Assert that the accessor format is as expected for the type?
    }

    _nameToAnimation[animationName] = std::move(animation);
}

const std::unique_ptr<Animation_controller_component::Animation>& 
Animation_controller_component::animationByName(const std::string& animationName)
{
    const auto pos = _nameToAnimation.find(animationName);
    if (pos == _nameToAnimation.end())
        throw std::logic_error("Animation doesn't exist with name: " + animationName);
    return pos->second;
}