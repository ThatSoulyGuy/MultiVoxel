#pragma once

#include "Client/Packet/RpcClient.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"

using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Client::Packet
{
    class ReplicationReceiver final : public PacketReceiver
    {

    public:

        void OnPacketReceived(const std::string& name, const std::string& data) override
        {
            if (name != SyncChannelName)
                return;

            std::istringstream stream(data);
            cereal::BinaryInputArchive archive(stream);

            uint32_t spawnCount;
            archive(spawnCount);

            while (spawnCount--)
            {
                NetworkId id;

                std::string gameObjectName;
                archive(id, gameObjectName);

                auto gameObject = GameObject::Create(IndexedString(gameObjectName));

                gameObject->SetNetworkId(id);
                gameObject->RemoveComponent<Transform>();

                GameObjectManager::GetInstance().Register(gameObject);

                localTree.AddNode(id, gameObjectName, 0);
            }

            uint32_t deletionCount;  archive(deletionCount);

            while (deletionCount--)
            {
                NetworkId id; archive(id);

                GameObjectManager::GetInstance().Unregister(id);

                localTree.RemoveNode(id);
            }

            uint32_t additionCount;
            archive(additionCount);

            while (additionCount--)
            {
                NetworkId parentId, childId;
                archive(parentId, childId);

                if (auto optP = GameObjectManager::GetInstance().Get(parentId))
                {
                    if (auto optC = GameObjectManager::GetInstance().Get(childId))
                        optP.value()->AddChild(optC.value());
                }

                localTree.AddEdge(parentId, childId);
            }

            uint32_t removalCount;
            archive(removalCount);

            while (removalCount--)
            {
                NetworkId parentId, childId;
                archive(parentId, childId);

                if (auto optP = GameObjectManager::GetInstance().Get(parentId))
                    optP.value()->RemoveChild(childId);

                localTree.RemoveEdge(parentId, childId);
            }

            uint32_t componentAdditionCount;
            archive(componentAdditionCount);

            while (componentAdditionCount--)
            {
                NetworkId objectId;

                std::string componentTypeName;

                archive(objectId, componentTypeName);

                if (auto optionalGameObject = GameObjectManager::GetInstance().Get(objectId))
                    optionalGameObject.value()->AddComponentDynamic(ComponentFactory::Create(componentTypeName));
            }

            uint32_t componentRemovalCount;
            archive(componentRemovalCount);

            while (componentRemovalCount--)
            {
                NetworkId objectId;

                std::string componentTypeName;

                archive(objectId, componentTypeName);

                if (auto optGO = GameObjectManager::GetInstance().Get(objectId))
                    optGO.value()->RemoveComponentDynamic(ComponentFactory::Create(componentTypeName));
            }

            uint32_t total;
            archive(total);

            while (true)
            {
                bool hasNext;

                archive(hasNext);

                if (!hasNext)
                    break;

                NetworkId id;

                std::string componentName;

                archive(id, componentName);

                if (auto optionalGameObject = GameObjectManager::GetInstance().Get(id))
                {
                    auto component = ComponentFactory::Create(componentName);

                    component = optionalGameObject.value()->AddComponentDynamic(component);

                    if (auto networkComponent = dynamic_cast<INetworkSerializable*>(component.get()))
                        networkComponent->Deserialize(archive);
                }
            }

            if (!localTree.Equals(LightTree::BuildFromManager()))
            {
                RpcClient::GetInstance().Reload();
                localTree.Clear();
            }
        }

        struct Node
        {
            IndexedString name;
            NetworkId parent{};

            std::vector<NetworkId> children;

            bool operator==(Node const& o) const noexcept
            {
                return name == o.name && parent == o.parent && children == o.children;
            }

            bool operator!=(Node const& o) const
            {
                return !(*this == o);
            }
        };

        class LightTree
        {

        public:

            void AddNode(NetworkId id, const std::string& name, NetworkId parent)
            {
                nodes[id] = Node{ IndexedString(name), parent, {} };
            }

            void RemoveNode(NetworkId id)
            {
                nodes.erase(id);
            }

            void AddEdge(NetworkId parentId, NetworkId childId)
            {
                nodes[parentId].children.push_back(childId);
                nodes[childId].parent = parentId;
            }

            void RemoveEdge(NetworkId parentId, NetworkId childId)
            {
                auto& children = nodes[parentId].children;

                std::erase(children, childId);

                nodes[childId].parent = 0;
            }

            bool Equals(const LightTree& other) const
            {
                return nodes == other.nodes;
            }

            void Clear()
            {
                nodes.clear();
            }

            bool operator==(LightTree const& o) const noexcept
            {
                return nodes == o.nodes;
            }

            bool operator!=(LightTree const& o) const noexcept
            {
                return !(*this == o);
            }

            static LightTree BuildFromManager()
            {
                LightTree result;

                auto& manager = GameObjectManager::GetInstance();

                for (auto& go : manager.GetAll())
                {
                    auto id = go->GetNetworkId();

                    result.nodes[id] = Node{ go->GetName(), go->GetParent().has_value() ? go->GetParent().value()->GetNetworkId() : 0, {}};
                }

                for (auto& [id, node] : result.nodes)
                {
                    if (node.parent && result.nodes.contains(node.parent))
                        result.nodes[node.parent].children.push_back(id);
                }

                return result;
            }

        private:

            std::unordered_map<NetworkId, Node> nodes;

            friend class std::hash<LightTree>;

        } localTree;
    };
}

namespace std
{
    template<>
    struct hash<MultiVoxel::Client::Packet::ReplicationReceiver::Node>
    {
        size_t operator()(MultiVoxel::Client::Packet::ReplicationReceiver::Node const& n) const noexcept
        {
            size_t seed = 0;

            HashCombine(seed, std::hash<IndexedString>()(n.name));

            HashCombine(seed, std::hash<NetworkId>()(n.parent));

            for (const auto childId : n.children)
                HashCombine(seed, std::hash<NetworkId>()(childId));

            return seed;
        }
    };

    template<>
    struct hash<MultiVoxel::Client::Packet::ReplicationReceiver::LightTree>
    {
        size_t operator()(MultiVoxel::Client::Packet::ReplicationReceiver::LightTree const& t) const noexcept
        {
            size_t seed = 0;

            for (auto const& [id, node] : t.nodes)
            {
                HashCombine(seed, std::hash<NetworkId>()(id));

                HashCombine(seed, std::hash<MultiVoxel::Client::Packet::ReplicationReceiver::Node>()(node));
            }

            return seed;
        }
    };
}