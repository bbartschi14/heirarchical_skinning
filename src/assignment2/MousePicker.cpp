#include "MousePicker.hpp"


#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/shaders/PhongShader.hpp"

namespace GLOO {
	MousePicker::MousePicker(Scene* scene, ArcBallCameraNode* camera, SkeletonNode* skeleton) : SceneNode() {
		scene_ptr_ = scene;
		camera_ptr_ = camera;
		skeleton_ptr_ = skeleton;
		projection_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetProjectionMatrix();
		view_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetViewMatrix();
		current_ray_ = glm::vec3(0.0f);
		camera_pos_ = glm::vec3(0.0f);
		sphere_hit_ = nullptr;
		auto selected_node = make_unique<SceneNode>();
		selected_node->CreateComponent<ShadingComponent>(std::make_shared<PhongShader>());
		selected_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(0.026f, 25, 25));
		glm::vec3 color(0.0f, 1.0f, 0.0f);
		auto material = std::make_shared<Material>(color, color, color, 0);
		selected_node->CreateComponent<MaterialComponent>(material);
		selected_node_ptr_ = selected_node.get();
		selected_node_ptr_->SetActive(false);
		AddChild(std::move(selected_node));
		rotating_ = false;
	}

	glm::vec3 MousePicker::GetCurrentRay() {
		return current_ray_;
	}

	void MousePicker::Update(double delta_time) {
		if (sphere_hit_ != nullptr) {
			selected_node_ptr_->GetTransform().SetPosition(sphere_hit_->GetTransform().GetWorldPosition());
		}

		static bool prev_released = true;
		if (InputManager::GetInstance().IsLeftMousePressed()) {
			if (prev_released) {
				view_matrix_ = scene_ptr_->GetActiveCameraPtr()->GetViewMatrix();
				current_ray_ = CalculateMouseRay();
				//CastRay(current_ray_);
				if (sphere_hit_ != nullptr && !rotating_) {					
					auto new_small_sphere = FindSphereHit(current_ray_, sphere_hit_->GetGizmoPtrs());
					if (new_small_sphere != nullptr) {
						current_rotating_node_ = new_small_sphere;
						rotating_ = true;
						mouse_start_click_ = InputManager::GetInstance().GetCursorPosition();
					}
					
				}
				
				if (!rotating_) {
					auto new_sphere = FindSphereHit(current_ray_, skeleton_ptr_->GetSpherePtrs());
					if (new_sphere != nullptr) {
						if (sphere_hit_ != nullptr) {
							int num_children = sphere_hit_->GetChildrenCount();
							for (int i = 0; i < num_children; i++) {
								sphere_hit_->GetChild(i).SetActive(false);
							}
						}

						sphere_hit_ = new_sphere;
						selected_node_ptr_->SetActive(true);
						selected_node_ptr_->GetTransform().SetPosition(sphere_hit_->GetTransform().GetWorldPosition());
						int num_children = sphere_hit_->GetChildrenCount();
						for (int i = 0; i < num_children; i++) {
							sphere_hit_->GetChild(i).SetActive(true);
						}
					}
				}
				
				prev_released = false;

			}
			else if (rotating_ && current_rotating_node_ != nullptr) {

				int axis = current_rotating_node_->GetGizmoAxis();
				RotateGizmo(InputManager::GetInstance().GetCursorPosition(), current_rotating_node_->GetParentPtr()->GetParentPtr(), axis);
			}
			
		} else {
			prev_released = true;
			rotating_ = false;
		}
		if (InputManager::GetInstance().IsKeyPressed(256)) {
			selected_node_ptr_->SetActive(false);
			if (sphere_hit_ != nullptr) {
				int num_children = sphere_hit_->GetChildrenCount();
				for (int i = 0; i < num_children; i++) {
					sphere_hit_->GetChild(i).SetActive(false);
				}
			}
		}
	}
	void MousePicker::RotateGizmo(glm::dvec2 pos, SceneNode* node, int axis) {
		float distance = mouse_start_click_.x - pos.x;
		float delta = 10.0f;
		if (distance != 0) {
			RotateJoint(node, axis, distance/delta);
			mouse_start_click_ = glm::dvec2(pos.x, pos.y);
		}
		

	}
	void MousePicker::RotateJoint(SceneNode* node, int axis, float angle) {
		//std::cout << "Rotating joint" << axis << std::endl;
		glm::vec3 rot_axis;
		if (axis == 1) {
			glm::vec4 new_axis = node->GetTransform().GetLocalToWorldMatrix() * glm::vec4(1.0f, 0.0f, 0.0f, 0.f);
			rot_axis = glm::vec3(new_axis[0], new_axis[1], new_axis[2]);
		}
		else if (axis == 2) {
			glm::vec4 new_axis = node->GetTransform().GetLocalToWorldMatrix() * glm::vec4(0.0f, 1.0f , 0.0f,0.f);
			rot_axis = glm::vec3(new_axis[0], new_axis[1], new_axis[2]);
		} 
		else if (axis == 0) {
			glm::vec4 new_axis = node->GetTransform().GetLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 1.0f, 0.f);
			rot_axis = glm::vec3(new_axis[0], new_axis[1], new_axis[2]);
		}
		
		glm::quat new_rot = glm::quat(cosf(angle / 2), rot_axis[0] * sinf(angle / 2), rot_axis[1] * sinf(angle / 2), rot_axis[2] * sinf(angle / 2));
		glm::quat original_rot = node->GetTransform().GetRotation();
		node->GetTransform().SetRotation(new_rot* original_rot );
		
		//node->GetTransform().SetRotation(rot_axis, angle);

		skeleton_ptr_->OnJointChanged(true);
	}

	void MousePicker::CastRay(glm::vec3 ray) {
		//std::cout << "Casting Ray" << std::endl;
		auto line = std::make_shared<VertexObject>();
		auto line_shader = std::make_shared<SimpleShader>();
		auto indices = IndexArray();
		indices.push_back(0);
		indices.push_back(1);
		line->UpdateIndices(make_unique<IndexArray>(indices));
		auto positions = make_unique<PositionArray>();
		glm::vec3 pos = camera_pos_;
		//std::cout << pos[0] << " " << pos[1] << " " << pos[2] << " " << std::endl;
		float length = 10.0f;

		auto sphere_node = make_unique<SceneNode>();
		sphere_node->CreateComponent<ShadingComponent>(std::make_shared<PhongShader>());
		sphere_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(0.005f, 25, 25));
		sphere_node->GetTransform().SetPosition(pos);
		AddChild(std::move(sphere_node));


		positions->push_back(pos);
		positions->push_back(pos +(ray*length));
		line->UpdatePositions(std::move(positions));

		auto line_node = make_unique<SceneNode>();
		line_node->CreateComponent<ShadingComponent>(line_shader);
		auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
		rc_line.SetDrawMode(DrawMode::Lines);
		auto color = glm::vec3(1.f, 0.f, 0.f);
		auto material = std::make_shared<Material>(color, color, color, 0.0f);
		line_node->CreateComponent<MaterialComponent>(material);
		AddChild(std::move(line_node));
	}

	SceneNode* MousePicker::FindSphereHit(glm::vec3 ray, std::vector<SceneNode*> nodes) {
		SceneNode* sphere_hit_ptr = nullptr;
		std::vector<float> sphere_hit_distances;
		std::vector<SceneNode*> sphere_hit_ptrs;
		bool hit_registered = false;
		for (auto sphere_ptr : nodes) {
			float sphere_dist = CheckCollision(ray, sphere_ptr);
			//std::cout << sphere_dist << std::endl;
			if (sphere_dist >= 0) {
				hit_registered = true;
				sphere_hit_distances.push_back(sphere_dist);
				sphere_hit_ptrs.push_back(sphere_ptr);
			}
		}
		if (hit_registered) {
			float nearest_dist = sphere_hit_distances[0];
			auto nearest_sphere = sphere_hit_ptrs[0];
			for (int i = 1; i < sphere_hit_distances.size(); i++) {
				if (sphere_hit_distances[i] < nearest_dist) {
					nearest_dist = sphere_hit_distances[i];
					nearest_sphere = sphere_hit_ptrs[i];
				}
			}
			sphere_hit_ptr = nearest_sphere;
		}
		
		return sphere_hit_ptr;

	}

	

	float MousePicker::CheckCollision(glm::vec3 ray, SceneNode* sphere) {
		float radius = .025f;
		glm::vec3 center = sphere->GetTransform().GetWorldPosition();
		//std::cout << "Center: " << center[0] << " " << center[1] << " " << center[2] << std::endl;
		//std::cout << "Ray: " << ray[0] << " " << ray[1] << " " << ray[2] << std::endl;
		//std::cout << "Origin: " << camera_pos_[0] << " " << camera_pos_[1] << " " << camera_pos_[2] << std::endl;

		glm::vec3 oc = camera_pos_ - center;
		float a = glm::dot(ray, ray);
		float b = 2.0f * glm::dot(oc, ray);
		float c = glm::dot(oc, oc) - (radius * radius);
		float discriminant = b * b - 4 * a * c;
		if (discriminant < 0) {
			return -1.0f;
		}
		else {
			return (-b - sqrt(discriminant)) / 2.0f * a;
		}


	}

	glm::vec3 MousePicker::CalculateMouseRay() {
		glm::vec2 mouse_pos = InputManager::GetInstance().GetCursorPosition();
		glm::vec2 normalized_coords = GetNormalizedDeviceCoords(mouse_pos);
		glm::vec4 clip_coords = glm::vec4(normalized_coords[0], normalized_coords[1], -1.0f, 1.0f);
		glm::vec4 eye_coords = GetEyeCoords(clip_coords);
		//std::cout << "Coords: " << eye_coords[0] << " " << eye_coords[1] << " " << eye_coords[2] << " " << eye_coords[3] << std::endl;

		glm::vec3 world_ray = GetWorldCoords(eye_coords);
		glm::vec3 ray = glm::normalize(world_ray);
		//std::cout << "Ray: " << ray[0] << " " << ray[1] << " " << ray[2] << std::endl;
	
		return ray;
	}

	glm::vec2 MousePicker::GetNormalizedDeviceCoords(glm::vec2 mouse_position) {
		glm::vec2 window_size = InputManager::GetInstance().GetWindowSize();
		float x = (2.0f * mouse_position[0]) / window_size[0] - 1.0f;
		float y = (2.0f * mouse_position[1]) / window_size[1] - 1.0f;
		return glm::vec2(x, -y);
	}

	glm::vec4 MousePicker::GetEyeCoords(glm::vec4 clip_coords) {
		glm::mat4 inverted_projection = glm::inverse(projection_matrix_);
		glm::vec4 eye_coords = inverted_projection * clip_coords;
		return glm::vec4(eye_coords[0], eye_coords[1], eye_coords[2], eye_coords[3]);
	}

	glm::vec3 MousePicker::GetWorldCoords(glm::vec4 eye_coords) {
		glm::mat4 inverted_view = glm::inverse(view_matrix_);

		camera_pos_ = glm::vec3(inverted_view[3][0], inverted_view[3][1], inverted_view[3][2]);
		glm::vec4 divided_eye = glm::vec4(eye_coords[0] / eye_coords[3], eye_coords[1] / eye_coords[3], eye_coords[2] / eye_coords[3], 0);
		glm::vec4 world_ray = inverted_view * divided_eye;
		glm::vec3 mouse_ray = glm::vec3(world_ray[0], world_ray[1], world_ray[2]);
		return mouse_ray;
	}
	

}