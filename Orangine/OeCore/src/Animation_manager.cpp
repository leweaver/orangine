#include "pch.h"

#include "Animation_manager.h"

#include "OeCore/Animation_controller_component.h"
#include "OeCore/Morph_weights_component.h"
#include "OeCore/Scene.h"

#include <algorithm>

using namespace oe;
using namespace DirectX;

std::string Animation_manager::_name = "Animation_manager";

template <> IAnimation_manager* oe::create_manager(Scene& scene)
{
  return new Animation_manager(scene);
}

const std::array<
    std::function<void(
        Entity&,
        const Animation_controller_component::Animation_channel& channel,
        uint32_t lowerValueIndex,
        uint32_t upperValueIndex,
        double factor)>,
    4>
    g_lerp_animation_type_handlers = {
        &Animation_manager::handleTranslationAnimationLerp,
        &Animation_manager::handleRotationAnimationLerp,
        &Animation_manager::handleScaleAnimationLerp,
        &Animation_manager::handleMorphAnimationLerp,
};

const std::array<
    std::function<void(
        Entity&,
        const Animation_controller_component::Animation_channel& channel,
        uint32_t lowerValueIndex)>,
    4>
    g_step_animation_type_handlers = {
        &Animation_manager::handleTranslationAnimationStep,
        &Animation_manager::handleRotationAnimationStep,
        &Animation_manager::handleScaleAnimationStep,
        &Animation_manager::handleMorphAnimationStep,
};

const std::array<
    std::function<void(
        Entity&,
        const Animation_controller_component::Animation_channel& channel,
        uint32_t lowerValueIndex,
        uint32_t upperValueIndex,
        double factor)>,
    4>
    g_cubic_spline_animation_type_handlers = {
        &Animation_manager::handleTranslationAnimationCubicSpline,
        &Animation_manager::handleRotationAnimationCubicSpline,
        &Animation_manager::handleScaleAnimationCubicSpline,
        &Animation_manager::handleMorphAnimationCubicSpline,
};

void Animation_manager::initialize()
{
  _animationControllers = _scene.manager<IScene_graph_manager>().getEntityFilter(
      {Animation_controller_component::type()});
}

void Animation_manager::shutdown() { _animationControllers = nullptr; }

const std::string& Animation_manager::name() const { return _name; }

void Animation_manager::tick()
{
  const auto deltaTime = _scene.deltaTime();
  for (const auto entity : *_animationControllers) {
    const auto animComponent = entity->getFirstComponentOfType<Animation_controller_component>();
    assert(animComponent);

    for (auto& activeAnimation : animComponent->activeAnimations) {
      const auto& name = activeAnimation.first;
      auto& channelStates = activeAnimation.second;

      const auto& animation = animComponent->animationByName(name);
      if (!animation) {
        LOG(WARNING) << "Missing animation with name: " << name;
        continue;
      }

      auto numComplete = 0u;
      for (size_t channelIdx = 0; channelIdx < animation->channels.size(); ++channelIdx) {
        const auto& animationChannel = animation->channels[channelIdx];

        if (channelIdx >= channelStates.size())
          continue;

        auto& state = channelStates[channelIdx];
        if (!state.playing)
          continue;

        const auto& keyframeTimes = *animationChannel->keyframeTimes;
        // We can assert this, as the channels are validated upon adding to the component.
        assert(!keyframeTimes.empty());

        unsigned minIndex, maxIndex = 0;
        // TODO: Looping over here is a pretty brute force way for working out which animation key
        // to use. Could store last used minIndex somewhere? How to deal with someone else modifying
        // currentTime?
        for (; maxIndex < keyframeTimes.size(); ++maxIndex) {
          if (keyframeTimes[maxIndex] > state.currentTime)
            break;
        }

        if (maxIndex == 0) {
          // Will reach this point if currentTime has not yet reached the beginning of animation.
          minIndex = 0;
        } else if (maxIndex == keyframeTimes.size()) {
          // Reach this point if current time is greater than the end of the animation
          if (keyframeTimes.size() > 1) {
            --maxIndex;
            minIndex = maxIndex - 1;
          } else {
            maxIndex = minIndex = 0;
          }
          ++numComplete;
        } else {
          // Normal case; at a valid point in an animation with >= 2 keyframes
          assert(keyframeTimes.size() > 1);
          minIndex = maxIndex - 1;
        }

        const auto duration = keyframeTimes[maxIndex] - keyframeTimes[minIndex];
        const auto factor =
            duration > 0 ? std::min(1.0, (state.currentTime - keyframeTimes[minIndex]) / duration)
                         : 0.0;
        auto animatedEntity = animationChannel->targetNode ? animationChannel->targetNode : entity;

        switch (animationChannel->interpolationType) {
        case Animation_interpolation::Linear: {
          const auto& fn = g_lerp_animation_type_handlers[static_cast<unsigned>(
              animationChannel->animationType)];
          fn(*animatedEntity, *animationChannel, minIndex, maxIndex, factor);
          break;
        }
        case Animation_interpolation::Step: {
          const auto& fn = g_step_animation_type_handlers[static_cast<unsigned>(
              animationChannel->animationType)];
          fn(*animatedEntity, *animationChannel, minIndex);
          break;
        }
        case Animation_interpolation::Cubic_spline: {
          const auto& fn = g_cubic_spline_animation_type_handlers[static_cast<unsigned>(
              animationChannel->animationType)];
          fn(*animatedEntity, *animationChannel, minIndex, maxIndex, factor);
          break;
        }
        default:
          OE_THROW(std::logic_error("Unsupported animation interpolation type"));
        }

        state.currentTime += deltaTime * state.speed;
      }

      if (numComplete == animation->channels.size()) {
        for (size_t channelIdx = 0; channelIdx < channelStates.size(); ++channelIdx)
          channelStates[channelIdx].currentTime = 0;
      }
    }
  }
}

template <class TTypeIn, class TTypeOut> void convertSplineElement(const TTypeIn& in, TTypeOut& out)
{
}

template <> void convertSplineElement(const Float3& in, SSE::Vector3& out)
{
  out = SSE::Vector3(in.x, in.y, in.z);
}
template <> void convertSplineElement(const Float4& in, SSE::Vector4& out)
{
  out = SSE::Vector4(in.x, in.y, in.z, in.w);
}
template <> void convertSplineElement(const Float4& in, SSE::Quat& out)
{
  out = SSE::Quat(in.x, in.y, in.z, in.w);
}

template <class TTypeIn, class TTypeOut>
const TTypeOut getKeyframeValue(
    const Animation_controller_component::Animation_channel& channel,
    uint32_t index)
{
  auto val = reinterpret_cast<const TTypeIn*>(channel.keyframeValues->getIndexed(index))[0];
  TTypeOut outVal;
  convertSplineElement(val, outVal);
  return outVal;
}

const float getKeyframeValueFloat(
    const Animation_controller_component::Animation_channel& channel,
    uint32_t index)
{
  return reinterpret_cast<const float*>(channel.keyframeValues->getIndexed(index))[0];
}

void Animation_manager::handleTranslationAnimationStep(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex)
{
  entity.setPosition(getKeyframeValue<Float3, SSE::Vector3>(channel, lowerValueIndex));
}

void Animation_manager::handleScaleAnimationStep(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex)
{
  entity.setScale(getKeyframeValue<Float3, SSE::Vector3>(channel, lowerValueIndex));
}

void Animation_manager::handleRotationAnimationStep(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex)
{
  entity.setRotation(getKeyframeValue<Float4, SSE::Quat>(channel, lowerValueIndex));
}

void Animation_manager::handleMorphAnimationStep(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex)
{
  handleMorphAnimationLerp(entity, channel, lowerValueIndex, lowerValueIndex, 0.0f);
}

void Animation_manager::handleTranslationAnimationLerp(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  entity.setPosition(SSE::lerp(
      static_cast<float>(factor),
      getKeyframeValue<Float3, SSE::Vector3>(channel, lowerValueIndex),
      getKeyframeValue<Float3, SSE::Vector3>(channel, upperValueIndex)));
}

void Animation_manager::handleScaleAnimationLerp(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  entity.setScale(SSE::lerp(
      static_cast<float>(factor),
      getKeyframeValue<Float3, SSE::Vector3>(channel, lowerValueIndex),
      getKeyframeValue<Float3, SSE::Vector3>(channel, upperValueIndex)));
}

void Animation_manager::handleRotationAnimationLerp(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  entity.setRotation(SSE::slerp(
      static_cast<float>(factor),
      getKeyframeValue<Float4, SSE::Quat>(channel, lowerValueIndex),
      getKeyframeValue<Float4, SSE::Quat>(channel, upperValueIndex)));
}

std::array<
    std::function<
        void(std::array<double, 8>& meshWeights, size_t meshWeightsOffset, const Float4& weights)>,
    5>
    g_applyWeights = {[](auto& meshWeights, auto meshWeightsOffset, const auto& weights) {},
                      [](auto& meshWeights, auto meshWeightsOffset, const auto& weights) {
                        meshWeights[meshWeightsOffset] = weights.x;
                      },
                      [](auto& meshWeights, auto meshWeightsOffset, const auto& weights) {
                        meshWeights[meshWeightsOffset] = weights.x;
                        meshWeights[meshWeightsOffset + 1] = weights.y;
                      },
                      [](auto& meshWeights, auto meshWeightsOffset, const auto& weights) {
                        meshWeights[meshWeightsOffset] = weights.x;
                        meshWeights[meshWeightsOffset + 1] = weights.y;
                        meshWeights[meshWeightsOffset + 2] = weights.z;
                      },
                      [](auto& meshWeights, auto meshWeightsOffset, const auto& weights) {
                        meshWeights[meshWeightsOffset] = weights.x;
                        meshWeights[meshWeightsOffset + 1] = weights.y;
                        meshWeights[meshWeightsOffset + 2] = weights.z;
                        meshWeights[meshWeightsOffset + 3] = weights.w;
                      }};

void Animation_manager::handleMorphAnimationLerp(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  const auto morphWeightsComponent = entity.getFirstComponentOfType<Morph_weights_component>();
  if (morphWeightsComponent) {
    const auto morphTargetCount = static_cast<uint32_t>(morphWeightsComponent->morphTargetCount());
    lowerValueIndex *= channel.valuesPerKeyFrame;
    upperValueIndex *= channel.valuesPerKeyFrame;

    auto lowerWeights = SSE::Vector4();
    auto upperWeights = SSE::Vector4();
    if (morphTargetCount <= 4) {
      for (uint8_t targetIdx = 0u; targetIdx < morphTargetCount; ++targetIdx) {
        lowerWeights.setElem(
            targetIdx, getKeyframeValueFloat(channel, lowerValueIndex + targetIdx));
        upperWeights.setElem(
            targetIdx, getKeyframeValueFloat(channel, upperValueIndex + targetIdx));
      }

      const auto weights = SSE::lerp(static_cast<float>(factor), lowerWeights, upperWeights);
      const auto numToApply = std::min(4u, morphTargetCount);
      g_applyWeights.at(
          numToApply)(morphWeightsComponent->morphWeights(), 0, static_cast<Float4>(weights));
    } else {
      assert(morphTargetCount <= 8);
      lowerWeights = getKeyframeValue<Float4, SSE::Vector4>(channel, lowerValueIndex);
      upperWeights = getKeyframeValue<Float4, SSE::Vector4>(channel, upperValueIndex);

      auto weights = SSE::lerp(static_cast<float>(factor), lowerWeights, upperWeights);
      g_applyWeights.at(4)(morphWeightsComponent->morphWeights(), 0, static_cast<Float4>(weights));

      for (uint8_t targetIdx = 4u; targetIdx < morphTargetCount; ++targetIdx) {
        lowerWeights.setElem(
            targetIdx, getKeyframeValueFloat(channel, lowerValueIndex + targetIdx));
        upperWeights.setElem(
            targetIdx, getKeyframeValueFloat(channel, upperValueIndex + targetIdx));
      }

      weights = SSE::lerp(static_cast<float>(factor), lowerWeights, upperWeights);
      const auto numToApply = std::min(4u, morphTargetCount - 4u);
      g_applyWeights.at(
          numToApply)(morphWeightsComponent->morphWeights(), 4, static_cast<Float4>(weights));
    }
  }
}

template <class TType> struct Cubic_spline_value {
  TType inTangent;
  TType value;
  TType outTangent;
};

template <class TTypeIn, class TTypeOut>
TTypeOut Animation_manager::calculateCubicSpline(
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  const auto& lower = reinterpret_cast<const Cubic_spline_value<TTypeIn>*>(
      channel.keyframeValues->getIndexed(lowerValueIndex * channel.valuesPerKeyFrame))[0];
  const auto& upper = reinterpret_cast<const Cubic_spline_value<TTypeIn>*>(
      channel.keyframeValues->getIndexed(upperValueIndex * channel.valuesPerKeyFrame))[0];

  const auto factor2 = factor * factor;
  const auto factor3 = factor2 * factor;

  // TODO: The splines should be pre-converted when loading the glTF
  TTypeOut lowerValue, outTangent, upperValue, inTangent;
  convertSplineElement(lower.value, lowerValue);
  convertSplineElement(lower.outTangent, outTangent);
  convertSplineElement(upper.value, upperValue);
  convertSplineElement(upper.inTangent, inTangent);

  // p(t) = (2t3 - 3t2 + 1)p0 + (t3 - 2t2 + t)m0 + (-2t3 + 3t2)p1 + (t3 - t2)m1
  const auto p0 = static_cast<float>((2.0 * factor3 - 3.0 * factor2 + 1.0)) * lowerValue;
  const auto m0 = static_cast<float>((factor3 - 2.0 * factor2 + factor)) * outTangent;
  const auto p1 = static_cast<float>((-2.0 * factor3 + 3.0 * factor2)) * upperValue;
  const auto m1 = static_cast<float>((factor3 - factor2)) * inTangent;

  auto result = p0 + m0 + p1 + m1;
  return result;
}

void Animation_manager::handleTranslationAnimationCubicSpline(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  entity.setPosition(calculateCubicSpline<Float3, SSE::Vector3>(
      channel, lowerValueIndex, upperValueIndex, factor));
}

void Animation_manager::handleScaleAnimationCubicSpline(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  entity.setScale(calculateCubicSpline<Float3, SSE::Vector3>(
      channel, lowerValueIndex, upperValueIndex, factor));
}

void Animation_manager::handleRotationAnimationCubicSpline(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  auto result =
      calculateCubicSpline<Float4, SSE::Quat>(channel, lowerValueIndex, upperValueIndex, factor);
  entity.setRotation(SSE::normalize(result));
}

void Animation_manager::handleMorphAnimationCubicSpline(
    Entity& entity,
    const Animation_controller_component::Animation_channel& channel,
    uint32_t lowerValueIndex,
    uint32_t upperValueIndex,
    double factor)
{
  // The one frame in the value stream is packed as such:
  // [ in-tangent-0 ... in-tangent-N, morph-value-0 ... morph-value-N, out-tangent-0 ...
  // out-tangent-N ]

  const auto morphWeightsComponent = entity.getFirstComponentOfType<Morph_weights_component>();
  if (morphWeightsComponent) {
    const auto morphTargetCount = static_cast<uint32_t>(morphWeightsComponent->morphTargetCount());
    lowerValueIndex *= channel.valuesPerKeyFrame;
    upperValueIndex *= channel.valuesPerKeyFrame;

    const auto weightsIdxOffset = morphTargetCount * 1;
    const auto inTangentIdxOffset = morphTargetCount * 2;
    const auto factor2 = factor * factor;
    const auto factor3 = factor2 * factor;
    assert(morphWeightsComponent->morphWeights().size() >= morphTargetCount);
    for (uint8_t targetIdx = 0u; targetIdx < morphTargetCount; ++targetIdx) {

      const auto lowerInTangent = reinterpret_cast<const float*>(
          channel.keyframeValues->getIndexed(lowerValueIndex))[targetIdx];
      const auto lowerValue = reinterpret_cast<const float*>(
          channel.keyframeValues->getIndexed(lowerValueIndex))[weightsIdxOffset + targetIdx];
      const auto upperValue = reinterpret_cast<const float*>(
          channel.keyframeValues->getIndexed(upperValueIndex))[weightsIdxOffset + targetIdx];
      const auto upperOutTangent = reinterpret_cast<const float*>(
          channel.keyframeValues->getIndexed(upperValueIndex))[inTangentIdxOffset + targetIdx];

      // p(t) = (2t3 - 3t2 + 1)p0 + (t3 - 2t2 + t)m0 + (-2t3 + 3t2)p1 + (t3 - t2)m1
      const auto p0 = static_cast<float>((2.0 * factor3 - 3.0 * factor2 + 1.0)) * lowerValue;
      const auto m0 = static_cast<float>((factor3 - 2.0 * factor2 + factor)) * lowerInTangent;
      const auto p1 = static_cast<float>((-2.0 * factor3 + 3.0 * factor2)) * upperValue;
      const auto m1 = static_cast<float>((factor3 - factor2)) * upperOutTangent;

      const auto result = p0 + m0 + p1 + m1;
      morphWeightsComponent->morphWeights()[targetIdx] = result;
    }
  }
}
