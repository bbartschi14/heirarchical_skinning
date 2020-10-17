#include "SkeletonNode.hpp"

#include "gloo/utils.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/MeshLoader.hpp"
#include "gloo/debug/PrimitiveFactory.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include <fstream>
#include <algorithm>

namespace GLOO {
SkeletonNode::SkeletonNode(const std::string& filename)
    : SceneNode(), draw_mode_(DrawMode::Skeleton) {
  LoadAllFiles(filename);
  DecorateTree();

  // Force initial update.
  OnJointChanged(false);
}

void SkeletonNode::ToggleDrawMode() {
  draw_mode_ =
      draw_mode_ == DrawMode::Skeleton ? DrawMode::SSD : DrawMode::Skeleton;
  // TODO: implement here toggling between skeleton mode and SSD mode.
  // The current mode is draw_mode_;
  // Hint: you may find SceneNode::SetActive convenient here as
  // inactive nodes will not be picked up by the renderer.

  if (draw_mode_ == DrawMode::SSD) {
      for (auto joint : joint_ptrs_) {
          joint->SetActive(false);
      }
      ssd_ptr_->SetActive(true);
  }
  else {
      for (auto joint : joint_ptrs_) {
          joint->SetActive(true);
      }
      ssd_ptr_->SetActive(false);
  }
  
}

std::vector<SceneNode*> SkeletonNode::GetSpherePtrs() {
    return sphere_nodes_ptrs_;
}

void SkeletonNode::DecorateTree() {
  // TODO: set up addtional nodes, add necessary components here.
  // You should create one set of nodes/components for skeleton mode
  // (spheres for joints and cylinders for bones), and another set for
  // SSD mode (you could just use a single node with a RenderingComponent
  // that is linked to a VertexObject with the mesh information. Then you
  // only need to update the VertexObject - updating vertex positions and
  // recalculating the normals, etc.).

    shader_ = std::make_shared<PhongShader>();
    sphere_mesh_ = PrimitiveFactory::CreateSphere(0.025f, 25, 25);
    cylinder_mesh_ = PrimitiveFactory::CreateCylinder(0.015f, 1, 25);
    std::vector<glm::vec3> origins;
    for (int i = 0; i < joint_ptrs_.size(); i++) {
        int child_count = joint_ptrs_[i]->GetChildrenCount();
        bool check_exists = std::find(origins.begin(), origins.end(), joint_ptrs_[i]->GetTransform().GetPosition()) != origins.end();
        bool check_zero = joint_ptrs_[i]->GetTransform().GetPosition() != glm::vec3(0.0f);
        if (!(check_exists) && check_zero) {
            origins.push_back(joint_ptrs_[i]->GetTransform().GetPosition());
            auto sphere_node = make_unique<SceneNode>();
            sphere_node->CreateComponent<ShadingComponent>(shader_);
            sphere_node->CreateComponent<RenderingComponent>(sphere_mesh_);
            auto sphere_node_ptr = sphere_node.get();
            sphere_nodes_ptrs_.push_back(sphere_node_ptr);
            joint_ptrs_[i]->AddChild(std::move(sphere_node));

            std::vector<glm::vec3> offsets;
            float scale = .075f;
            offsets.push_back(glm::vec3(1.0f * scale, 0.0f, 0.0f));
            offsets.push_back(glm::vec3(0.0f, 1.0f * scale, 0.0f));
            offsets.push_back(glm::vec3(0.0f, 0.0f, 1.0f * scale));

            std::vector<glm::vec3> color;
            color.push_back(glm::vec3(1.0f, 0.f, 0.f));
            color.push_back(glm::vec3(0.0f, 1.f, 0.f));
            color.push_back(glm::vec3(0.0f, 0.f, 1.f));

            for (int num = 0; num < 3; num++) {
                auto small_sphere_node = make_unique<SceneNode>();
                small_sphere_node->CreateComponent<ShadingComponent>(shader_);
                small_sphere_node->CreateComponent<RenderingComponent>(PrimitiveFactory::CreateSphere(0.01f, 25, 25));
                small_sphere_node->GetTransform().SetPosition(offsets[num]);
                auto material = std::make_shared<Material>(color[num], color[num], color[num], 0);
                small_sphere_node->CreateComponent<MaterialComponent>(material);
                small_sphere_node->SetActive(false);
                small_sphere_node->SetGizmoAxis(num);

                auto line = std::make_shared<VertexObject>();
                auto line_shader = std::make_shared<SimpleShader>();
                auto indices = IndexArray();
                indices.push_back(0);
                indices.push_back(1);
                line->UpdateIndices(make_unique<IndexArray>(indices));
                auto positions = make_unique<PositionArray>();
                float length = .05f;
                if (num == 0) {
                    positions->push_back(glm::vec3(0.f, -1.f * length, 0.f));
                    positions->push_back(glm::vec3(0.f, 1.f * length, 0.f));
                }
                else if (num == 1) {
                    positions->push_back(glm::vec3(0.f, 0.f, -1.f * length));
                    positions->push_back(glm::vec3(0.f, 0.f, 1.f * length));
                }
                else {
                    positions->push_back(glm::vec3(-1.f * length, 0.f, 0.f));
                    positions->push_back(glm::vec3(1.f * length, 0.f, 0.f));
                }
                
                line->UpdatePositions(std::move(positions));

                auto line_node = make_unique<SceneNode>();
                line_node->CreateComponent<ShadingComponent>(line_shader);
                auto& rc_line = line_node->CreateComponent<RenderingComponent>(line);
                rc_line.SetDrawMode(GLOO::DrawMode::Lines);
                auto material_new = std::make_shared<Material>(color[num], color[num], color[num], 0.0f);
                line_node->CreateComponent<MaterialComponent>(material);


                small_sphere_node->AddChild(std::move(line_node));




                sphere_node_ptr->AddToGizmoPtrs(small_sphere_node.get());
                sphere_node_ptr->AddChild(std::move(small_sphere_node));
            }
        }
        
        std::vector<glm::vec3> child_origins;
        for (int j = 0; j < child_count; j++) {
            
            bool check_child_exists = std::find(child_origins.begin(), child_origins.end(), joint_ptrs_[i]->GetChild(j).GetTransform().GetPosition()) != child_origins.end();
            bool check_child_zero = joint_ptrs_[i]->GetChild(j).GetTransform().GetPosition() != glm::vec3(0.0f);

            if (!check_child_exists && check_child_zero) {
                child_origins.push_back(joint_ptrs_[i]->GetChild(j).GetTransform().GetPosition());
                auto cylinder_node = make_unique<SceneNode>();
                cylinder_node->CreateComponent<ShadingComponent>(shader_);
                cylinder_node->CreateComponent<RenderingComponent>(cylinder_mesh_);

                glm::vec3 joint_pos = joint_ptrs_[i]->GetChild(j).GetTransform().GetPosition();
                float height = glm::length(joint_pos);

                cylinder_node->GetTransform().SetScale(glm::vec3(1.0f, height, 1.0f));

                glm::vec3 rot_axis = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), glm::normalize(joint_pos)));
                float rot_angle = glm::acos(glm::dot(glm::vec3(0.f, 1.f, 0.f), glm::normalize(joint_pos)));
                cylinder_node->GetTransform().SetRotation(rot_axis, rot_angle);

                cylinder_nodes_ptrs_.push_back(cylinder_node.get());
                joint_ptrs_[i]->AddChild(std::move(cylinder_node));
            }
        }
    }
    
    
    auto mesh_node = make_unique<SceneNode>();
    mesh_node->CreateComponent<ShadingComponent>(shader_);
    mesh_node->CreateComponent<RenderingComponent>(bind_pose_mesh_);
    mesh_node->CreateComponent<MaterialComponent>(std::make_shared<Material>(Material::GetDefault()));
    glm::vec3 color = glm::vec3(.7f);
    mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetAmbientColor(color);
    mesh_node->GetComponentPtr<MaterialComponent>()->GetMaterial().SetDiffuseColor(color);

    ssd_ptr_ = mesh_node.get();
    
    AddChild(std::move(mesh_node));
    ssd_ptr_->SetActive(false);

    
}

void SkeletonNode::Update(double delta_time) {
  // Prevent multiple toggle.
  static bool prev_released = true;
  if (InputManager::GetInstance().IsKeyPressed('S')) {
    if (prev_released) {
      ToggleDrawMode();
    }
    prev_released = false;
  } else if (InputManager::GetInstance().IsKeyReleased('S')) {
    prev_released = true;
  }
}

void SkeletonNode::OnJointChanged(bool from_gizmo) {
  // TODO: this method is called whenever the values of UI sliders change.
  // The new Euler angles (represented as EulerAngle struct) can be retrieved
  // from linked_angles_ (a std::vector of EulerAngle*).
  // The indices of linked_angles_ align with the order of the joints in .skel
  // files. For instance, *linked_angles_[0] corresponds to the first line of
  // the .skel file.
    
    if (!from_gizmo) {
        if (linked_angles_.size() > 0) {
            for (int i = 0; i < joint_ptrs_.size(); i++) {

                glm::vec3 rot_vec = glm::vec3(linked_angles_[i]->rx, linked_angles_[i]->ry, linked_angles_[i]->rz);

                joint_ptrs_[i]->GetTransform().SetRotation(glm::quat(rot_vec));
            }
        }
    }
    
    
    CalculateTMatrices();
    ComputeNewPositions();
    CalculateNormals();
}

void SkeletonNode::LinkRotationControl(const std::vector<EulerAngle*>& angles) {
  linked_angles_ = angles;
}

void SkeletonNode::ComputeNewPositions() {
    auto new_positions = make_unique<PositionArray>();

    for (int i = 0; i < orig_positions_.size(); i++) {
        glm::vec4 new_pos = glm::vec4(0.0f);
        for (int j = 0; j < joint_ptrs_.size() - 1; j++) {
            new_pos += vertex_weights_[i][j]*t_matrices[j]*b_matrices[j]*glm::vec4(orig_positions_[i],1.0f);
        }
        new_positions->push_back(glm::vec3(new_pos[0],new_pos[1],new_pos[2]));
    }
    bind_pose_mesh_->UpdatePositions(std::move(new_positions));
}

void SkeletonNode::FindIncidentTriangles() {
    auto indices = bind_pose_mesh_->GetIndices();
    auto positions = bind_pose_mesh_->GetPositions();
    std::vector<std::vector<int>> incident_tris;
    for (int position_index = 0; position_index < positions.size(); position_index++) {
        std::vector<int> tris;
        for (int i = 0; i < indices.size() - 2; i += 3) {
            if (indices[i] == position_index || indices[i + 1] == position_index || indices[i + 2] == position_index) {
                tris.push_back(i / 3);
            }
        }
        incident_tris.push_back(tris);
    }
    incident_triangles_ = incident_tris;
}

void SkeletonNode::CalculateTMatrices() {
    t_matrices.clear();
    glm::mat4 mat = joint_ptrs_[1]->GetTransform().GetLocalToWorldMatrix();
    //std::cout << "Original Matrix: " << mat[0][0] << " " << mat[0][1] << " " << mat[0][2] << " " << mat[0][3] << " " << mat[1][0] << " " << mat[1][1] << " " << mat[1][2] << " " << mat[1][3] << std::endl;
    for (int i = 1; i < joint_ptrs_.size(); i++) {
        t_matrices.push_back(joint_ptrs_[i]->GetTransform().GetLocalToWorldMatrix());
    }
    glm::mat4 mat_new = t_matrices[0];
    //std::cout << "New Matrix: " << mat_new[0][0] << " " << mat_new[0][1] << " " << mat_new[0][2] << " " << mat_new[0][3] << " " << mat_new[1][0] << " " << mat_new[1][1] << " " << mat_new[1][2] << " " << mat_new[1][3] << std::endl;

}

void SkeletonNode::CalculateBMatrices() {
    for (int i = 1; i < joint_ptrs_.size(); i++) {
        b_matrices.push_back(glm::inverse(joint_ptrs_[i]->GetTransform().GetLocalToWorldMatrix()));
    }
}

void SkeletonNode::CalculateNormals() {
    auto indices = bind_pose_mesh_->GetIndices();
    auto positions = bind_pose_mesh_->GetPositions();
    auto new_normals = make_unique<NormalArray>();
    for (int position_index = 0; position_index < positions.size(); position_index++) {
        glm::vec3 vertex_norm = glm::vec3(0.0f);
        for (int tri : incident_triangles_[position_index]) {
            glm::vec3 a = positions[indices[(tri * 3)]];
            glm::vec3 b = positions[indices[(tri * 3)+1]];
            glm::vec3 c = positions[indices[(tri * 3)+2]];

            glm::vec3 u = b - a;
            glm::vec3 v = c - a;

            glm::vec3 tri_norm = glm::normalize(glm::cross(u, v));

            vertex_norm += tri_norm * FindTriArea(a, b, c);
        }
        new_normals->push_back(glm::normalize(vertex_norm));
    }
    
    bind_pose_mesh_->UpdateNormals(std::move(new_normals));
}

float SkeletonNode::FindTriArea(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    float u_mag = glm::length(glm::cross(b - a, c - a));
    return u_mag / 2;
}
void SkeletonNode::LoadSkeletonFile(const std::string& path) {
    std::vector<glm::vec3> joint_positions;
    std::vector<int> joint_parents;

    std::ifstream infile(path);
    float x, y, z;
    int parent_index;
    while (infile >> x >> y >> z >> parent_index)
    {
        //std::cout << x << " " << y << " " << z << " " << parent_index << std::endl;
        joint_positions.push_back(glm::vec3(x, y, z));
        joint_parents.push_back(parent_index);
    }

    joint_ptrs_.resize(joint_parents.size());
    RecursiveAddJoints(*this, -1, joint_positions, joint_parents);

    
    //std::cout << joint_ptrs_[1]->GetChildrenCount() << std::endl;

}

void SkeletonNode::RecursiveAddJoints(SceneNode& parent, int parent_index, std::vector<glm::vec3> positions, std::vector<int> joint_parents) {
    int current_child = 0;
    //std::cout << "Finding " << parent_index << "'s" << std::endl;

    for (int i = 0; i < joint_parents.size(); i++) {
        if (joint_parents[i] == parent_index) {
            //std::cout << "Found " << i << " for " << parent_index << std::endl;
            auto joint_node = make_unique<SceneNode>();
            joint_node->GetTransform().SetPosition(positions[i]);
            joint_ptrs_[i] = joint_node.get();
            parent.AddChild(std::move(joint_node));
            auto& new_ptr = parent.GetChild(current_child);
            current_child++;
            RecursiveAddJoints(new_ptr, i, positions, joint_parents);
        }
    }
}

void SkeletonNode::LoadMeshFile(const std::string& filename) {
  std::shared_ptr<VertexObject> vtx_obj =
      MeshLoader::Import(filename).vertex_obj;
  bind_pose_mesh_ = vtx_obj;
  orig_positions_ = bind_pose_mesh_->GetPositions();
  FindIncidentTriangles();
  CalculateNormals();
  CalculateBMatrices();
}

void SkeletonNode::LoadAttachmentWeights(const std::string& path) {
    std::vector<std::vector<float>> weights;
    std::ifstream infile(path);
    float single_weight;
    
    std::vector<float> single_line;
    while (infile >> single_weight) {
        single_line.push_back(single_weight);
        //std::cout << single_weight << " ";

        if (single_line.size() == joint_ptrs_.size() - 1) {
            //std::cout << single_line.size() << std::endl;

            weights.push_back(single_line);
            single_line.clear();
        }
    }
    //std::cout << weights.size() << std::endl;
    vertex_weights_ = weights;


}

void SkeletonNode::LoadAllFiles(const std::string& prefix) {
  std::string prefix_full = GetAssetDir() + prefix;
  LoadSkeletonFile(prefix_full + ".skel");
  LoadMeshFile(prefix + ".obj");
  LoadAttachmentWeights(prefix_full + ".attach");
}
}  // namespace GLOO
