#pragma once

#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PeerConnection.hpp"
#include "Server/RPC/RpcTypes.hpp"
#include "Server/PermissionManager.hpp"

using namespace MultiVoxel::Independent::Core;
using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Network;
using namespace MultiVoxel::Server::Rpc;
using namespace MultiVoxel::Server;

namespace MultiVoxel::Server::Packet
{
    class RpcReceiver
    {

    public:

        void HandleRpc(PeerConnection& peer, const std::string& data)
        {
            HSteamNetConnection who = peer.GetHandle();

            std::istringstream is(data);
            cereal::BinaryInputArchive ar(is);

            uint32_t count;
            ar(count);

            while (count--)
            {
                uint64_t callId;
                RpcType rpc;

                ar(callId, rpc);

                //if (!PermissionManager::GetInstance().HasPermission(who, rpc))
                //    continue;

                switch (rpc)
                {
                    case RpcType::CreateGameObject:
                    {
                        std::string name; NetworkId parentId;
                        ar(name, parentId);
                        HandleCreate(peer, callId, name, parentId);
                        break;
                    }

                    case RpcType::DestroyGameObject:
                    {
                        NetworkId objId; ar(objId);
                        HandleDestroy(objId);
                        break;
                    }

                    case RpcType::AddChild:
                    {
                        NetworkId parent, child;
                        ar(parent, child);
                        HandleAddChild(parent, child);
                        break;
                    }

                    case RpcType::RemoveChild:
                    {
                        NetworkId parent, child;
                        ar(parent, child);
                        HandleRemoveChild(parent, child);
                        break;
                    }
                }
            }
        }

        static RpcReceiver& Create()
        {
            static RpcReceiver inst;

            return inst;
        }

    private:

        RpcReceiver() = default;

        void HandleCreate(PeerConnection& peer, uint64_t callId, const std::string& name, NetworkId parentId)
        {
            auto go = GameObject::Create(IndexedString(name));

            if (auto optParent = GameObjectManager::GetInstance().Get(parentId))
                optParent.value()->AddChild(go);

            GameObjectManager::GetInstance().Register(go);

            Settings::GetInstance().REPLICATION_SENDER.Get()->QueueSpawn(go);

            std::ostringstream os;
            cereal::BinaryOutputArchive rar(os);

            rar(std::string("RpcChannel"));
            rar(uint32_t(1));
            rar(callId, RpcType::CreateGameObjectResponse, go->GetNetworkId());

            auto const& buf = os.str();
            auto msg = Message::Create(Message::Type::Custom, buf.data(), buf.size(), true);

            peer.Send(msg);

            std::cout << "Sent message with buffer '" << buf << "' to peer '" << peer.GetHandle() << "' with callId '" << callId << "'." << std::endl;
        }

        void HandleDestroy(NetworkId id)
        {
            if (auto opt = GameObjectManager::GetInstance().Get(id))
                GameObjectManager::GetInstance().Unregister(id);
        }

        void HandleAddChild(NetworkId parentId, NetworkId childId)
        {
            auto& mgr = GameObjectManager::GetInstance();

            if (auto p = mgr.Get(parentId))
            {
                if (auto c = mgr.Get(childId))
                    p.value()->AddChild(c.value());
            }
        }

        void HandleRemoveChild(NetworkId parentId, NetworkId childId)
        {
            auto& mgr = GameObjectManager::GetInstance();

            if (auto p = mgr.Get(parentId))
                p.value()->RemoveChild(childId);
        }
    };
}