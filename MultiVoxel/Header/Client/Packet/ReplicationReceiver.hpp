#pragma once

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
            cereal::BinaryInputArchive ar(stream);

            uint32_t spawnCount;
            ar(spawnCount);

            while (spawnCount--)
            {
                NetworkId nid;

                std::string nm;
                ar(nid, nm);

                auto go = GameObject::Create(IndexedString(nm));

                go->SetNetworkId(nid);
                go->RemoveComponent<Transform>();

                GameObjectManager::GetInstance().Register(go);
            }

            uint32_t total;
            
            ar(total);

            while (true)
            {
                bool hasNext;

                ar(hasNext);

                if (!hasNext)
                    break;

                NetworkId nid;

                std::string compName;

                ar(nid, compName);

                auto optGO = GameObjectManager::GetInstance().Get(nid);

                if (!optGO)
                    continue;

                auto comp = ComponentFactory::Create(compName);

                if (comp)
                    optGO.value()->AddComponentDynamic(comp);
                else
                {
                    std::cerr << "Unknown component type: " << compName << "\n";

                    return;
                }

                if (auto net = dynamic_cast<INetworkSerializable*>(comp.get()))
                    net->Deserialize(ar);
            }
        }
    };
}