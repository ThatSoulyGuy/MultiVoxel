#pragma once

#include "Client/Packet/RpcClient.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"

using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Client::Packet
{
    class ReplicationReceiver : public PacketReceiver
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
                std::string name;

                archive(id, name);

                auto gameObject = GameObject::Create(IndexedString(name));

                gameObject->SetNetworkId(id);

                gameObject->RemoveComponent<Transform>();

                GameObjectManager::GetInstance().Register(gameObject);

                localTree.AddNode(id, name, 0);
            }

            uint32_t deletionCount;
            archive(deletionCount);

            while (deletionCount--)
            {
                NetworkId id;
                archive(id);

                GameObjectManager::GetInstance().Unregister(id);

                localTree.RemoveNode(id);
            }

            uint32_t additionCount;
            archive(additionCount);

            while (additionCount--)
            {
                NetworkId parentId, childId;
                
                archive(parentId, childId);

                auto optionalParent = GameObjectManager::GetInstance().Get(parentId);
                auto optionalChild = GameObjectManager::GetInstance().Get(childId);

                if (optionalParent && optionalChild)
                    optionalParent.value()->AddChild(optionalChild.value());

                localTree.AddEdge(parentId, childId);
            }

            uint32_t removalCount;
            
            archive(removalCount);

            while (removalCount--)
            {
                NetworkId parentId, childId;
                
                archive(parentId, childId);

                auto optionalParent = GameObjectManager::GetInstance().Get(parentId);

                if (optionalParent)
                    optionalParent.value()->RemoveChild(childId);

                localTree.RemoveEdge(parentId, childId);
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

                auto optionalGameObject = GameObjectManager::GetInstance().Get(id);

                if (!optionalGameObject)
                    continue;

                auto component = ComponentFactory::Create(componentName);
                optionalGameObject.value()->AddComponentDynamic(component);

                if (auto networkComponent = dynamic_cast<INetworkSerializable*>(component.get()))
                    networkComponent->Deserialize(archive);
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
            NetworkId parent;

            std::vector<NetworkId> children;

            bool operator==(Node const& o) const noexcept
            {
                return name == o.name && parent == o.parent && children == o.children;
            }

            bool operator!=(Node const& o)
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

                children.erase(std::remove(children.begin(), children.end(), childId), children.end());

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
                    if (node.parent && result.nodes.count(node.parent))
                        result.nodes[node.parent].children.push_back(id);
                }

                return result;
            }

        private:

            std::unordered_map<NetworkId, Node> nodes;

            friend class std::hash<MultiVoxel::Client::Packet::ReplicationReceiver::LightTree>;

        } localTree;
    };
}

namespace std
{
    inline void hash_combine(std::size_t& seed, std::size_t h) noexcept
    {
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<>
    struct hash<MultiVoxel::Client::Packet::ReplicationReceiver::Node>
    {
        size_t operator()(MultiVoxel::Client::Packet::ReplicationReceiver::Node const& n) const noexcept
        {
            size_t seed = 0;

            hash_combine(seed, std::hash<IndexedString>()(n.name));

            hash_combine(seed, std::hash<NetworkId>()(n.parent));

            for (auto cid : n.children)
                hash_combine(seed, std::hash<NetworkId>()(cid));

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
                hash_combine(seed, std::hash<NetworkId>()(id));

                hash_combine(seed, std::hash<MultiVoxel::Client::Packet::ReplicationReceiver::Node>()(node));
            }

            return seed;
        }
    };
}