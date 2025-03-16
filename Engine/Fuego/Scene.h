#pragma once

#include <fupch.h>
#include <list>
#include "glm/glm.hpp"

namespace Fuego
{
class Material;
class Scene;

class BaseSceneObject
{
public:
    BaseSceneObject(Scene* master_scene, const std::string& name, bool enabled);
    virtual ~BaseSceneObject() = default;

    inline const std::string& GetName() const
    {
        return name;
    }
private:
    std::string name;
    bool enabled;
    Scene* master_scene;
};
class SceneFolder : public BaseSceneObject
{
public:
    SceneFolder(Scene* master_scene, const std::string& folder_name);
};
class SceneObject : public BaseSceneObject
{
public:
    SceneObject(Scene* master_scene, const std::string & name, glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f));

private:
    glm::vec3 position;
    glm::vec3 rotation;
};
class ModelObject : public SceneObject
{
public:
    ModelObject(Scene* master_scene, const std::string & name, glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f), const Material* mat = nullptr);

private:
    const Material* material;
};

class Node
{
public:
    template<typename T>
    Node(Node* parent, T* obj)
        : parent(parent)
        , object(obj)
    {
        FU_CORE_TRACE("Node ctor");
        id = Node::GetID();
    }
    ~Node() = default;

    std::list<Node>& GetChildren()
    {
        return children;
    }
    template <typename T>
    inline T* GetSceneObject()
    {
        return object;
    }

    void PrintNode() const;
    inline const Node* GetParent() const
    {
        return parent;
    }

    
    template <typename T>
    Node* AddChild(T scene_obj)
    {
        return &children.emplace_back(this, scene_obj);
    }

private:
    Node* parent;
    static uint32_t GetID();

    BaseSceneObject* object;

    std::list<Node> children;

    uint32_t id;
    static uint32_t s_id;
};

class Scene
{

public:
    Scene(const std::string& new_scene_obj);
    ~Scene();

    /// <summary>
    /// qq ww
    /// </summary>
    /// <param name="value">eeeee</param>
    /// <returns>wwww</returns>
    template <typename T, typename... Args>
        requires std::is_base_of_v<BaseSceneObject, T>
    T* AddObject(BaseSceneObject* parent, Args&&... args)
    {
        T* scene_obj = new T(this, std::forward<Args>(args)...);
        Node* new_node = nullptr;
        if (parent)
        {
            Node* parent_node = object_to_node_map[parent];
            new_node = parent_node->AddChild(scene_obj);
        }
        else
        {
            new_node = new Node(nullptr, scene_obj);
        }
        object_to_node_map.emplace(scene_obj, new_node);
        objects_amount++;

        return scene_obj;
    }

    BaseSceneObject* FindObject(const std::string& object_name) const;
    Node* FindNode(const std::string& object_name) const;

    BaseSceneObject* Root();

    void SaveSceneToFile(const std::string& file_name);

private:

    SceneFolder* root_obj;
    Node* root_node;
    std::string scene_name;
    std::string scene_version = "1.0";
    uint16_t objects_amount;
    std::unordered_map<BaseSceneObject*, Node*> object_to_node_map;
};

}  // namespace Fuego::Editor