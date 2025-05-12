#pragma once

#include <ranges>

#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/PacketSender.hpp"

using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Server::Packet
{
    class ReplicationSender final : public PacketSender
    {

    public:

        void QueueSpawn(const std::shared_ptr<GameObject>& go)
        {
            spawnQueue.push_back(go);
        }

        void QueueDelete(const NetworkId id)
        {
            deleteQueue.push_back(id);
        }

        void QueueAddChild(NetworkId parent, NetworkId child)
        {
            addChildQueue.emplace_back(parent, child);
        }

        void QueueRemoveChild(NetworkId parent, NetworkId child)
        {
            removeChildQueue.emplace_back(parent, child);
        }

        void QueueAddComponent(NetworkId objectId, const std::string& compTypeName)
        {
            addComponentQueue.emplace_back(objectId, compTypeName);
        }
        void QueueRemoveComponent(NetworkId objectId, const std::string& compTypeName)
        {
            removeComponentQueue.emplace_back(objectId, compTypeName);
        }

        void Reload() override
        {
            sent = false;

            spawnQueue.clear();
            deleteQueue.clear();
            addChildQueue.clear();
            removeChildQueue.clear();

            for (auto& gameObject : GameObjectManager::GetInstance().GetAll())
                spawnQueue.push_back(gameObject);

            for (const auto& gameObject : GameObjectManager::GetInstance().GetAll())
            {
                for (const auto& component : gameObject->GetComponentMap() | std::views::values)
                {
                    if (const auto networkComponent = dynamic_cast<INetworkSerializable*>(component.get()))
                        networkComponent->MarkDirty();
                }
            }
        }

        bool SendPacket(std::string& outName, std::string& outData) override
        {
            if (sent)
                return false;

            std::ostringstream stream;
            cereal::BinaryOutputArchive archive(stream);

            archive(static_cast<uint32_t>(spawnQueue.size()));

            for (const auto& gameObject : spawnQueue)
                archive(gameObject->GetNetworkId(), gameObject->GetName().operator std::string());

            spawnQueue.clear();

            archive(static_cast<uint32_t>(deleteQueue.size()));

            for (auto id : deleteQueue)
                archive(id);

            deleteQueue.clear();

            archive(static_cast<uint32_t>(addChildQueue.size()));

            for (auto& [parentId, childId] : addChildQueue)
                archive(parentId, childId);

            addChildQueue.clear();

            archive(static_cast<uint32_t>(removeChildQueue.size()));

            for (auto& [parentId, childId] : removeChildQueue)
                archive(parentId, childId);

            removeChildQueue.clear();

            archive(static_cast<uint32_t>(addComponentQueue.size()));

            for (auto& [id, type] : addComponentQueue)
                archive(id, type);

            addComponentQueue.clear();

            archive(static_cast<uint32_t>(removeComponentQueue.size()));

            for (auto& [id, type] : removeComponentQueue)
                archive(id, type);

            removeComponentQueue.clear();

            const auto gameObjectList = GameObjectManager::GetInstance().GetAll();

            archive(static_cast<uint32_t>(gameObjectList.size()));

            for (auto& gameObject : gameObjectList)
            {
                for (const auto& component : gameObject->GetComponentMap() | std::views::values)
                {
                    if (const auto networkComponent = dynamic_cast<INetworkSerializable*>(component.get()); networkComponent && networkComponent->IsDirty())
                    {
                        archive(true, gameObject->GetNetworkId(), networkComponent->GetComponentTypeName());

                        networkComponent->Serialize(archive);
                        networkComponent->ClearDirty();
                    }
                }
            }

            archive(false);

            outName = SyncChannelName;
            outData = stream.str();

            sent = true;

            return true;
        }

    private:

        bool sent = false;

        std::vector<std::shared_ptr<GameObject>> spawnQueue;
        std::vector<NetworkId> deleteQueue;
        std::vector<std::pair<NetworkId, NetworkId>> addChildQueue;
        std::vector<std::pair<NetworkId, NetworkId>> removeChildQueue;
        std::vector<std::pair<NetworkId, std::string>> addComponentQueue;
        std::vector<std::pair<NetworkId, std::string>> removeComponentQueue;

    };
}