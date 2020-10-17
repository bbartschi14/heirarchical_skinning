#ifndef MOUSE_PICKER_H_
#define MOUSE_PICKER_H_

#include "gloo/SceneNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/cameras/ArcBallCameraNode.hpp"
#include "gloo/Scene.hpp"
#include "SkeletonNode.hpp"

#include <string>
#include <vector>

namespace GLOO {
    class MousePicker : public SceneNode {
    public:

        MousePicker(Scene* scene, ArcBallCameraNode* camera, SkeletonNode* skel_ptr);
        glm::vec3 GetCurrentRay();
        void Update(double delta_time) override;

    private:
        void CastRay(glm::vec3 ray);
        glm::vec3 CalculateMouseRay();
        glm::vec2 GetNormalizedDeviceCoords(glm::vec2 mouse_position);
        glm::vec4 GetEyeCoords(glm::vec4 clip_coords);
        glm::vec3 GetWorldCoords(glm::vec4 eye_coords);
        SceneNode* FindSphereHit(glm::vec3 ray, std::vector<SceneNode*> nodes);
        void RotateJoint(SceneNode* node, int axis, float amount);
        float CheckCollision(glm::vec3 ray, SceneNode* sphere);
        void RotateGizmo(glm::dvec2 pos, SceneNode* node, int axis);
        bool rotating_;
        SceneNode* current_rotating_node_;
        Scene* scene_ptr_;
        ArcBallCameraNode* camera_ptr_;
        SkeletonNode* skeleton_ptr_;
        SceneNode* sphere_hit_;
        SceneNode* small_sphere_hit_;
        glm::dvec2 mouse_start_click_;
        glm::mat4 projection_matrix_;
        glm::mat4 view_matrix_;
        glm::vec3 camera_pos_;
        glm::vec3 current_ray_;
        SceneNode* selected_node_ptr_;
    };
}  // namespace GLOO

#endif

