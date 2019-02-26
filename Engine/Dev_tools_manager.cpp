#include "pch.h"
#include "Dev_tools_manager.h"
#include "Primitive_mesh_data_factory.h"
#include "Renderable.h"
#include "Renderer_data.h"
#include "Unlit_material.h"
#include "Scene.h"
#include "imgui.h"
#include "VectorLog.h"
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "Animation_controller_component.h"

using namespace DirectX;
using namespace SimpleMath;
using namespace oe;
using namespace internal;

template<>
IDev_tools_manager* oe::create_manager(Scene & scene)
{
    return new Dev_tools_manager(scene);
}

void Dev_tools_manager::initialize()
{
	_noLightProvider = [](const BoundingSphere&, std::vector<Entity*>&, uint32_t) {};
    _animationControllers = _scene.manager<IScene_graph_manager>().getEntityFilter({ Animation_controller_component::type() });
}

void Dev_tools_manager::shutdown()
{
	assert(!_unlitMaterial);
	_debugShapes.resize(0);
    _animationControllers = nullptr;
}

void Dev_tools_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{
	_unlitMaterial = std::make_shared<Unlit_material>();
}

void Dev_tools_manager::destroyDeviceDependentResources()
{
	_unlitMaterial.reset();
	for (auto& debugShape : _debugShapes) {
		std::get<Renderable>(debugShape).rendererData.reset();
        std::get<Renderable>(debugShape).materialContext.reset();
	}
}

void Dev_tools_manager::addDebugSphere(const Matrix& worldTransform, float radius, const Color& color)
{
	Renderable renderable = {
		Primitive_mesh_data_factory::createSphere(radius, 6),
		_unlitMaterial,
		nullptr
	};
	_debugShapes.push_back({ worldTransform, color, std::move(renderable)});
}

void Dev_tools_manager::addDebugBoundingBox(const BoundingOrientedBox& boundingOrientedBox, const Color& color)
{
	auto worldTransform = XMMatrixAffineTransformation(Vector3::One, Vector3::Zero, XMLoadFloat4(&boundingOrientedBox.Orientation), XMLoadFloat3(&boundingOrientedBox.Center));
	
	Renderable renderable = {
		Primitive_mesh_data_factory::createBox(boundingOrientedBox.Extents * 2.0f),
		_unlitMaterial,
		nullptr
	};
	_debugShapes.push_back({ worldTransform, color, std::move(renderable) });
}

void Dev_tools_manager::addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const Color& color)
{
	Renderable renderable = {
		Primitive_mesh_data_factory::createFrustumLines(boundingFrustum),
		_unlitMaterial,
		nullptr
	};
	_debugShapes.push_back({ Matrix::Identity, color, std::move(renderable) });
}

void Dev_tools_manager::clearDebugShapes()
{
	_debugShapes.clear();
}

void Dev_tools_manager::renderDebugShapes(const Render_pass::Camera_data& cameraData)
{
	auto& entityRenderManager = _scene.manager<IEntity_render_manager>();
	for (auto& debugShape : _debugShapes) {
		const auto& transform = std::get<Matrix>(debugShape);
		const auto& color = std::get<Color>(debugShape);
		auto& renderable = std::get<Renderable>(debugShape);

		_unlitMaterial->setBaseColor(color);
		entityRenderManager.renderRenderable(renderable, transform, 0.0f, cameraData, _noLightProvider, Render_pass_blend_mode::Opaque, true);
	}
}

void Dev_tools_manager::renderImGui()
{
    // Display contents in a scrolling region
    auto scrollAmount = 0.0f;

    ImGui::SetNextWindowSize(ImVec2(554, 675), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Animation")) {
        for (const auto entity : *_animationControllers) {
            const auto animComponent = entity->getFirstComponentOfType<Animation_controller_component>();
            assert(animComponent);

            if (ImGui::CollapsingHeader(animComponent->entity().getName().c_str())) {
                if (!animComponent->activeAnimations.empty()) {
                    if (ImGui::Button("Reset All Times")) {
                        for (const auto& animEntry : animComponent->animations()) {
                            auto activeAnimationPos = animComponent->activeAnimations.find(animEntry.first);
                            if (activeAnimationPos == animComponent->activeAnimations.end())
                                continue;

                            for (auto i = 0; i < activeAnimationPos->second.size(); ++i) {
                                activeAnimationPos->second[i].currentTime = 0.0;
                            }
                        }
                    }
                }

                for (const auto& animEntry : animComponent->animations()) {
                    if (ImGui::TreeNode(animEntry.first.c_str())) {

                        auto activeAnimationPos = animComponent->activeAnimations.find(animEntry.first);
                        auto isActive = activeAnimationPos != animComponent->activeAnimations.end();

                        if (ImGui::Checkbox("Loaded?", &isActive)) {
                            if (isActive) {
                                animComponent->activeAnimations[animEntry.first] = {};

                                // Create the entry. States will be added in the next step.
                                decltype(animComponent->activeAnimations)::value_type entry = { animEntry.first , {} };

                                activeAnimationPos = animComponent->activeAnimations.insert(std::move(entry)).first;
                            }
                            else
                            {
                                animComponent->activeAnimations.erase(animEntry.first);
                                activeAnimationPos = animComponent->activeAnimations.end();
                            }
                        }

                        if (activeAnimationPos != animComponent->activeAnimations.end()) {

                            if (!activeAnimationPos->second.empty()) {
                                auto maxTime = 0.0f;
                                for (const auto &channel : animEntry.second->channels) {
                                    maxTime = std::max(maxTime, channel->keyframeTimes->back());
                                }

                                const auto firstTime = activeAnimationPos->second.front().currentTime;
                                auto showResetButton = false;
                                for (auto i = 1; i < activeAnimationPos->second.size(); ++i) {
                                    const auto& state = activeAnimationPos->second[i];
                                    if (state.playing && 
                                        (state.currentTime > firstTime + DBL_EPSILON || state.currentTime < firstTime - DBL_EPSILON)) {
                                        showResetButton = true;
                                        break;
                                    }
                                }

                                if (showResetButton) {
                                    if (ImGui::Button("Zero Timeline")) {
                                        for (auto i = 0; i < activeAnimationPos->second.size(); ++i) {
                                            activeAnimationPos->second[i].currentTime = 0.0;
                                        }
                                    }
                                }
                                else 
                                {
                                    auto currentTime = static_cast<float>(firstTime);
                                    if (ImGui::SliderFloat("Time", &currentTime, 0.0f, maxTime, "%.2f")) {
                                        for (auto i = 0; i < activeAnimationPos->second.size(); ++i) {
                                            activeAnimationPos->second[i].currentTime = currentTime;
                                        }
                                    }
                                }
                            }
                            
                            if (ImGui::TreeNode("Channels")) {
                                for (auto i = 0; i < animEntry.second->channels.size(); ++i) {
                                    const auto& channel = animEntry.second->channels[i];
                                    const auto animationTypeStr = animationTypeToString(channel->animationType);

                                    if (activeAnimationPos->second.size() <= i) {
                                        activeAnimationPos->second.resize(animEntry.second->channels.size());
                                    }

                                    auto& state = activeAnimationPos->second[i];
                                    ImGui::Checkbox(animationTypeStr.c_str(), &state.playing);
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
        }        
    }
    ImGui::End();

    if (ImGui::Begin("Console"))
    {
        auto uiScale = ImGui::GetIO().FontGlobalScale;
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Logs");
        if (ImGui::BeginChild("scrolling", ImVec2(0, -25.0f * uiScale), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGuiListClipper clipper;
            const auto numLines = _vectorLog->messageCount();
            clipper.Begin(numLines);
            while (clipper.Step())
            {
                for (auto line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    auto& logMessage = _vectorLog->getMessageAt(line_no);
                    const auto cStr = logMessage.message.c_str();
                    ImGui::TextUnformatted(cStr, cStr + logMessage.message.size());
                }
            }
            clipper.End();
            if (_scrollLogToBottom)
                ImGui::SetScrollHereY(1.0f);

            ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("commands", ImVec2(0, 20.0f * uiScale), false))
        {
            if (ImGui::InputText(">", &_consoleInput)) {
                if (_consoleInput.size()) {
                    if (_scene.manager<IEntity_scripting_manager>().commandSuggestions(_consoleInput, _commandSuggestions)) {
                        // TODO: Show suggestions
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Make some log!")) {
                _scene.manager<IEntity_scripting_manager>().execute(_consoleInput);
            }
            ImGui::SameLine(ImGui::GetWindowWidth() - 30 * uiScale);
            ImGui::Checkbox("Auto Scroll", &_scrollLogToBottom);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

