#ifndef SKELETON_NODE_H_
#define SKELETON_NODE_H_

#include "gloo/SceneNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/shaders/ShaderProgram.hpp"

#include <string>
#include <vector>

namespace GLOO {
class SkeletonNode : public SceneNode {
 public:
  enum class DrawMode { Skeleton, SSD };
  struct EulerAngle {
    float rx, ry, rz;
  };

  SkeletonNode(const std::string& filename);
  void LinkRotationControl(const std::vector<EulerAngle*>& angles);
  void Update(double delta_time) override;
  void OnJointChanged(bool from_gizmo);
  std::vector<SceneNode*> GetSpherePtrs();
  

 private:
  void LoadAllFiles(const std::string& prefix);
  void LoadSkeletonFile(const std::string& path);
  void LoadMeshFile(const std::string& filename);
  void LoadAttachmentWeights(const std::string& path);
  void RecursiveAddJoints(SceneNode& parent, int parent_index, std::vector<glm::vec3> positions, std::vector<int> joint_parents);
  void ToggleDrawMode();
  void DecorateTree();
  void CalculateNormals();
  void CalculateBMatrices();
  void CalculateTMatrices();
  void ComputeNewPositions();
  void FindIncidentTriangles();
  float FindTriArea(glm::vec3 a, glm::vec3 b, glm::vec3 c);
  DrawMode draw_mode_;
  // Euler angles of the UI sliders.
  std::vector<EulerAngle*> linked_angles_;
  std::vector<SceneNode*> joint_ptrs_;
  std::vector<SceneNode*> sphere_nodes_ptrs_;
  std::vector<SceneNode*> cylinder_nodes_ptrs_;
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<glm::mat4> b_matrices;
  std::vector<glm::mat4> t_matrices;
  PositionArray orig_positions_;
  std::shared_ptr<VertexObject> sphere_mesh_;
  std::shared_ptr<VertexObject> cylinder_mesh_;
  std::shared_ptr<VertexObject> bind_pose_mesh_;
  std::vector<std::vector<int>> incident_triangles_;
  SceneNode*  ssd_ptr_;
  std::shared_ptr<ShaderProgram> shader_;

};
}  // namespace GLOO

#endif
