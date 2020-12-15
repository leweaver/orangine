﻿#include "pch.h"

#include "OeCore/Render_pass_skybox.h"
#include "OeCore/Renderable.h"
#include "OeCore/Scene.h"
#include "OeCore/Skybox_material.h"

using oe::Render_pass_skybox;

Render_pass_skybox::Render_pass_skybox(Scene& scene) : _scene(scene) {
  _renderable = std::make_unique<Renderable>();
  _renderable->meshData = Primitive_mesh_data_factory::createSphere(1.0f, 3);

  _material = std::make_shared<Skybox_material>();
  _renderable->material = _material;
}

void Render_pass_skybox::render(const Camera_data& cameraData) {
  Camera_data skyboxCamera;

  skyboxCamera.enablePixelShader = cameraData.enablePixelShader;
  skyboxCamera.projectionMatrix =
      SSE::Matrix4::perspective(cameraData.fov, cameraData.aspectRatio, 0.01f, 1.0f);

  // Discard the position
  skyboxCamera.viewMatrix = SSE::Matrix4(cameraData.viewMatrix.getUpper3x3(), SSE::Vector3(0));

  _material->setCubeMapTexture(_scene.environmentVolume().environmentIbl.skyboxTexture);
  _scene.manager<IEntity_render_manager>().renderRenderable(
      *_renderable,
      SSE::Matrix4::identity(),
      0.0f,
      skyboxCamera,
      Light_provider::no_light_provider,
      Render_pass_blend_mode::Opaque,
      false);
}
