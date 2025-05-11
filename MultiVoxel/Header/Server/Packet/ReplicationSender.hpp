#pragma once

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

        void QueueDelete(NetworkId id)
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

        void Reload() override
        {
            sent = false;

            spawnQueue.clear();
            deleteQueue.clear();
            addChildQueue.clear();
            removeChildQueue.clear();

            for (auto& go : GameObjectManager::GetInstance().GetAll())
                spawnQueue.push_back(go);

            for (auto& go : GameObjectManager::GetInstance().GetAll())
            {
                for (auto& [ti, comp] : go->GetComponentMap())
                {
                    if (auto net = dynamic_cast<INetworkSerializable*>(comp.get()))
                        net->MarkDirty();
                }
            }
        }

        bool SendPacket(std::string& outName, std::string& outData) override
        {
            if (sent) return false;

            std::ostringstream os;
            cereal::BinaryOutputArchive ar(os);

            ar(uint32_t(spawnQueue.size()));

            for (auto& go : spawnQueue)
                ar(go->GetNetworkId(), go->GetName().operator std::string());

            spawnQueue.clear();

            ar(uint32_t(deleteQueue.size()));

            for (auto id : deleteQueue)
                ar(id);

            deleteQueue.clear();

            ar(uint32_t(addChildQueue.size()));

            for (auto& [p, c] : addChildQueue)
                ar(p, c);

            addChildQueue.clear();

            ar(uint32_t(removeChildQueue.size()));

            for (auto& [p, c] : removeChildQueue)
                ar(p, c);

            removeChildQueue.clear();

            auto all = GameObjectManager::GetInstance().GetAll();

            ar(uint32_t(all.size()));

            for (auto& go : all)
            {
                for (auto& [ti, comp] : go->GetComponentMap())
                {
                    if (auto net = dynamic_cast<INetworkSerializable*>(comp.get()); net && net->IsDirty())
                    {
                        ar(true, go->GetNetworkId(), net->GetComponentTypeName());

                        net->Serialize(ar);
                        net->ClearDirty();
                    }
                }
            }

            ar(false);

            outName = SyncChannelName;
            outData = os.str();

            sent = true;

            return true;
        }

    private:

        bool sent = false;

        std::vector<std::shared_ptr<GameObject>> spawnQueue;
        std::vector<NetworkId> deleteQueue;
        std::vector<std::pair<NetworkId, NetworkId>> addChildQueue;
        std::vector<std::pair<NetworkId, NetworkId>> removeChildQueue;

    };
}