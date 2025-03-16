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

    root = AddObject<SceneFolder>(nullptr, "Root Folder");
    
}
Scene::~Scene()
{
    FU_CORE_TRACE("Scene destructor");
    delete root;
}

BaseSceneObject* Scene::FindObject(const std::string& object_name) const
{
   /* auto it = objects_map.find(object_name);
    if (it != objects_map.end())
    {
        return it->second->GetSceneObject();
    }*/
    return nullptr;
}
Node* Scene::FindNode(const std::string& object_name) const
{
    /*auto it = objects_map.find(object_name);
    if (it != objects_map.end())
    {
        return it->second;
    }*/
    return nullptr;
}
BaseSceneObject* Scene::Root()
{
    //return static_cast<SceneFolder*>(root->GetSceneObject());
    return static_cast<BaseSceneObject*>(root);
}
void Scene::SaveSceneToFile(const std::string& file_name)
{
    auto& fs = Fuego::Application::Get().FileSystem();
    std::string scene_file_name = file_name + ".fu_scene";
    fs.FUCreateFile(scene_file_name, "Scenes");
}

BaseSceneObject::BaseSceneObject(const std::string& name, bool enabled, Scene* master_scene)
    : name(name)
    , enabled(enabled)
    , master_scene(master_scene)
{
    FU_CORE_INFO("BaseSceneObject: {0} ctor", this->name);
}

SceneFolder::SceneFolder(const std::string& folder_name, Scene* master_scene)
    : BaseSceneObject(folder_name, true, master_scene)
{
    FU_CORE_INFO("SceneFolder: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
SceneObject::SceneObject(const std::string& name, glm::vec3 pos, glm::vec3 rot, Scene* master_scene)
    : BaseSceneObject(name, true, master_scene)
    , position(pos)
    , rotation(rot)
{
    FU_CORE_INFO("SceneObject: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
ModelObject::ModelObject(const std::string& name, glm::vec3 pos, glm::vec3 rot, const Material* mat, Scene* master_scene)
    : SceneObject(name, pos, rot, master_scene)
    , material(mat)
{
    FU_CORE_INFO("ModelObject: {0} ctor", ((BaseSceneObject*)this)->GetName());
}
    
void Node::PrintNode() const
{
    // TODO
}

}  // namespace Fuego::Editor