
#include <cassert>
#include <iostream>
#include <glad/glad.h>
#include <glm/gtx/string_cast.hpp>

#include "Application.hpp"
#include "Scene.hpp"
#include "utils.hpp"
#include "gl_wrapper/BindGuard.hpp"
#include "shaders/ShaderProgram.hpp"
#include "components/ShadingComponent.hpp"
#include "components/CameraComponent.hpp"
#include "debug/PrimitiveFactory.hpp"


namespace GLOO {
Renderer::Renderer(Application& application) : application_(application) {
  UNUSED(application_);
}

void Renderer::SetRenderingOptions() const {
  GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

  // Enable depth test.
  GL_CHECK(glEnable(GL_DEPTH_TEST));
  GL_CHECK(glDepthFunc(GL_LEQUAL));

  // Enable blending for multi-pass forward rendering.
  GL_CHECK(glEnable(GL_BLEND));
  GL_CHECK(glBlendFunc(GL_ONE, GL_ONE));
}

void Renderer::Render(const Scene& scene) const {
  SetRenderingOptions();
  RenderScene(scene);
}

Renderer::RenderingInfo Renderer::RetrieveRenderingInfo(const Scene& scene) const {
    RenderingInfo info;
    
    const SceneNode& root = scene.GetRootNode();
    const glm::mat4 model_matrix = root.GetTransform().GetLocalToParentMatrix();
    RecursiveRetrieve(root, info, model_matrix);

    return info;
}

void Renderer::RecursiveRetrieve(const SceneNode& node, RenderingInfo& info, const glm::mat4& model_matrix) const{
    int num_children = node.GetChildrenCount();
    for (int i = 0; i < num_children; i++) {
        auto &child_ptr = node.GetChild(i);
        glm::mat4 child_matrix = model_matrix * child_ptr.GetTransform().GetLocalToParentMatrix();
        if (node.IsActive()) {
            auto rendering_ptr = child_ptr.GetComponentPtr<RenderingComponent>();
            if (rendering_ptr != nullptr) {
                info.emplace_back(rendering_ptr, child_matrix);
            }
        }

        RecursiveRetrieve(child_ptr, info, child_matrix);
    }

}

void Renderer::RenderScene(const Scene& scene) const {
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  const SceneNode& root = scene.GetRootNode();
  auto rendering_info = RetrieveRenderingInfo(scene);

  auto light_ptrs = root.GetComponentPtrsInChildren<LightComponent>();
  if (light_ptrs.size() == 0) {
    // Make sure there are at least 2 passes of we don't forget to set color
    // mask back.
    return;
  }

  CameraComponent* camera = scene.GetActiveCameraPtr();

  // First pass: depth buffer.
  // Remaining passes: one per light source.
  size_t total_passes = 1 + light_ptrs.size();

  for (size_t pass = 0; pass < total_passes; pass++) {

    GL_CHECK(glDepthMask((pass == 0) ? GL_TRUE : GL_FALSE));
    bool color_mask = (pass == 0) ? GL_FALSE : GL_TRUE;
    GL_CHECK(glColorMask(color_mask, color_mask, color_mask, color_mask));

    for (const auto& pr : rendering_info) {

      auto robj_ptr = pr.first;

      SceneNode& node = *robj_ptr->GetNodePtr();

      auto shading_ptr = node.GetComponentPtr<ShadingComponent>();

      if (shading_ptr == nullptr) {
        std::cerr << "Some mesh is not attached with a shader during rendering!"
                  << std::endl;
        continue;
      }

      ShaderProgram* shader = shading_ptr->GetShaderPtr();

      BindGuard shader_bg(shader);

      // Set various uniform variables in the shaders.
      shader->SetTargetNode(node, pr.second);
      shader->SetCamera(*camera);

      if (pass > 0) {
        LightComponent& light = *light_ptrs.at(total_passes - pass - 1);
        shader->SetLightSource(light);
      }

      robj_ptr->Render();

    }

  }

  // Re-enable writing to depth buffer.
  GL_CHECK(glDepthMask(GL_TRUE));

}


}  // namespace GLOO
