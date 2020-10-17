#include "SkeletonViewerApp.hpp"

#include "gloo/external.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/lights/AmbientLight.hpp"
#include "gloo/lights/DirectionalLight.hpp"
#include "gloo/components/LightComponent.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"

namespace {
const std::vector<std::string> kJointNames = {"Root",
                                              "Chest",
                                              "Waist",
                                              "Neck",
                                              "Right hip",
                                              "Right leg",
                                              "Right knee",
                                              "Right foot",
                                              "Left hip",
                                              "Left leg",
                                              "Left knee",
                                              "Left foot",
                                              "Right collarbone",
                                              "Right shoulder",
                                              "Right elbow",
                                              "Left collarbone",
                                              "Left shoulder",
                                              "Left elbow"};
}

namespace GLOO {

SkeletonViewerApp::SkeletonViewerApp(const std::string& app_name,
                                     glm::ivec2 window_size,
                                     const std::string& model_prefix)
    : Application(app_name, window_size),
      slider_values_(kJointNames.size(), {0.f, 0.f, 0.f}),
      model_prefix_(model_prefix) {
}

void SkeletonViewerApp::SetupScene() {
  SceneNode& root = scene_->GetRootNode();

  auto camera_node = make_unique<ArcBallCameraNode>(45.f, 1.6f, 3.0f);
  scene_->ActivateCamera(camera_node->GetComponentPtr<CameraComponent>());
  auto camera_ptr = camera_node.get();
  root.AddChild(std::move(camera_node));

  auto ambient_light = std::make_shared<AmbientLight>();
  ambient_light->SetAmbientColor(glm::vec3(0.2f));
  root.CreateComponent<LightComponent>(ambient_light);

  auto sun_light = std::make_shared<DirectionalLight>();
  sun_light->SetDiffuseColor(glm::vec3(0.6f, 0.6f, 0.6f));
  sun_light->SetSpecularColor(glm::vec3(0.6f, 0.6f, 0.6f));
  sun_light->SetDirection(glm::vec3(-1.0f, -0.8f, -1.0f));
  auto sun_light_node = make_unique<SceneNode>();
  sun_light_node->CreateComponent<LightComponent>(sun_light);
  root.AddChild(std::move(sun_light_node));

  auto skeletal_node = make_unique<SkeletonNode>(model_prefix_);
  skeletal_node_ptr_ = skeletal_node.get();
  root.AddChild(std::move(skeletal_node));

  std::vector<SkeletonNode::EulerAngle*> angles;
  for (size_t i = 0; i < slider_values_.size(); i++) {
    angles.push_back(&slider_values_[i]);
  }
  skeletal_node_ptr_->LinkRotationControl(angles);

  auto mouse_picker_node = make_unique<MousePicker>(scene_.get(), camera_ptr,skeletal_node_ptr_);
  root.AddChild(std::move(mouse_picker_node));

  auto quad_node = make_unique<SceneNode>();
  quad_node->CreateComponent<ShadingComponent>(std::make_shared<PhongShader>());
  quad_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateQuad());
  quad_node->GetTransform().SetRotation(glm::vec3(1.0f,0.f,0.f), 90);
  root.AddChild(std::move(quad_node));
}

void SkeletonViewerApp::DrawGUI() {
  bool modified = false;
  ImGui::Begin("Control Panel");
  ImGui::Text("Click on a joint to show rotation gizmo, then click and drag on an axis handle to rotate joint.");
  ImGui::Text("Esc deselects the joint");
  for (size_t i = 0; i < kJointNames.size(); i++) {
    ImGui::Text("%s", kJointNames[i].c_str());
    ImGui::PushID((int)i);
    modified |= ImGui::SliderFloat("x", &slider_values_[i].rx, -kPi, kPi);
    modified |= ImGui::SliderFloat("y", &slider_values_[i].ry, -kPi, kPi);
    modified |= ImGui::SliderFloat("z", &slider_values_[i].rz, -kPi, kPi);
    ImGui::PopID();
  }
  ImGui::End();

  if (modified) {
    skeletal_node_ptr_->OnJointChanged(false);
  }
}
}  // namespace GLOO
