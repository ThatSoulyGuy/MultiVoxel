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

        void Reset()
        {
            sent = false;
        }

        void QueueSpawn(const std::shared_ptr<GameObject>& go)
        {
            spawnQueue.push_back(go);
        }

        void Reload() override
        {
            Reset();

            for (auto& gameObject : GameObjectManager::GetInstance().GetAll())
                QueueSpawn(gameObject);

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
            if (sent)
                return false;

            std::ostringstream stream;

            cereal::BinaryOutputArchive archive(stream);

            archive(uint32_t(spawnQueue.size()));

            for (auto& go : spawnQueue)
                archive(go->GetNetworkId(), go->GetName().operator std::string());

            spawnQueue.clear();

            auto all = GameObjectManager::GetInstance().GetAll();

            archive(uint32_t(all.size()));

            for (auto& go : all)
            {
                for (auto& [ti, comp] : go->GetComponentMap())
                {
                    if (auto net = dynamic_cast<INetworkSerializable*>(comp.get()); net && net->IsDirty())
                    {
                        archive(true, go->GetNetworkId(), net->GetComponentTypeName());

                        net->Serialize(archive);
                        net->ClearDirty();
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

        std::vector<std::shared_ptr<GameObject>> spawnQueue;

    };
}