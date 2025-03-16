#include "Scene.h"

#include "../FileSystem/FileSystem.h"
#include "Application.h"

#pragma region Concepts

#pragma endregion

namespace Fuego
{
uint32_t Node::s_id = 0;

uint32_t Node::GetID()
{
    // TODO: to think about race conditions. std::atomic / mutex
    return ++s_id;
}

Scene::Scene(const std::string& scene_name)
    : scene_name(scene_name)
    , objects_amount(0)
{
    FU_CORE_TRACE("Scene ctor");

    root_obj = AddObject<SceneFolder>(nullptr, "Root Folder");
    root_node = object_to_node_map[root_obj];
    
}
Scene::~Scene()
{
    FU_CORE_TRACE("Scene destructor");
    delete root_obj;
}

BaseSceneObject* Scene::FindObject(const std::string& object_name) const
{
    // TODO
    return nullptr;
}
Node* Scene::FindNode(const std::string& object_name) const
{
    // TODO
    return nullptr;
}
BaseSceneObject* Scene::Root()
{
    return static_cast<BaseSceneObject*>(root_obj);
}
void Scene::SaveSceneToFile(const std::string& file_name)
{
    auto& fs = Fuego::Application::Get().FileSystem();
    std::string scene_file_name = file_name + ".fu_scene";
    fs.FUCreateFile(scene_file_name, "Scenes");
}

BaseSceneObject::BaseSceneObject(Scene* master_scene, const std::string& name, bool enabled)
    : name(name)
    , enabled(enabled)
    , master_scene(master_scene)
{
    FU_CORE_INFO("BaseSceneObject: {0} ctor", this->name);
}

SceneFolder::SceneFolder(Scene* master_scene, const std::string& folder_name)
    : BaseSceneObject(master_scene, folder_name, true)
{
    FU_CORE_INFO("SceneFolder: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
SceneObject::SceneObject(Scene* master_scene, const std::string& name, glm::vec3 pos, glm::vec3 rot)
    : BaseSceneObject(master_scene, name, true)
    , position(pos)
    , rotation(rot)
{
    FU_CORE_INFO("SceneObject: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
ModelObject::ModelObject(Scene* master_scene, const std::string& name, glm::vec3 pos, glm::vec3 rot, const Material* mat)
    : SceneObject(master_scene, name, pos, rot)
    , material(mat)
{
    FU_CORE_INFO("ModelObject: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
    
void Node::PrintNode() const
{
    // TODO
}

}  // namespace Fuego::Editor