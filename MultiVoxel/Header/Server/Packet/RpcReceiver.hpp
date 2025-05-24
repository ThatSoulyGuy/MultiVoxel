#pragma once

#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/NetworkManager.hpp"
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
    class RpcReceiver final
    {

    public:

        static void HandleRpc(PeerConnection& peer, const std::string& data)
        {
            const HSteamNetConnection requester = peer.GetHandle();

            std::istringstream stream(data);
            cereal::BinaryInputArchive archive(stream);

            uint32_t requestCount;
            archive(requestCount);

            while (requestCount--)
            {
                uint64_t callId;
                RpcType rpc;

                archive(callId, rpc);

                //if (!PermissionManager::GetInstance().HasPermission(requester, rpc))
                //    continue;

                switch (rpc)
                {
                    case RpcType::CreateGameObject:
                    {
                        std::string name;
                        NetworkId parentId;

                        archive(name, parentId);
                        HandleCreate(peer, callId, name, parentId);

                        break;
                    }

                    case RpcType::DestroyGameObject:
                    {
                        NetworkId objId;

                        archive(objId);
                        HandleDestroy(objId);

                        break;
                    }

                    case RpcType::AddChild:
                    {
                        NetworkId parent, child;

                        archive(parent, child);
                        HandleAddChild(parent, child);

                        break;
                    }

                    case RpcType::RemoveChild:
                    {
                        NetworkId parent, child;

                        archive(parent, child);
                        HandleRemoveChild(parent, child);

                        break;
                    }

                    case RpcType::RequestFullSync:
                        Settings::GetInstance().REPLICATION_SENDER.Get()->Reload();
                        break;

                    case RpcType::AddComponent:
                    {
                        std::string compTypeName;
                        NetworkId objectId;
                        std::string payload;

                        archive(compTypeName, objectId, payload);
                        HandleAddComponent(peer, callId, compTypeName, objectId, payload);

                        break;
                    }

                    case RpcType::RemoveComponent:
                    {
                        std::string compTypeName;
                        NetworkId objectId;

                        archive(compTypeName, objectId);
                        HandleRemoveComponent(peer, 0, compTypeName, objectId);

                        break;
                    }

                    case RpcType::MoveGameObjectRequest:
                    {
                        NetworkId id;
                        std::string payload;

                        archive(id, payload);

                        auto [position, rotation] = DeserializePositionRotation(payload);

                        if (auto gameObject = GameObjectManager::GetInstance().Get(id))
                        {
                            gameObject.value()->GetTransform()->SetLocalPosition(position, false);
                            gameObject.value()->GetTransform()->SetLocalRotation(rotation, false);
                        }

                        BroadcastTransformUpdate(id, position, rotation);

                        break;
                    }

                    default:
                        break;
                }
            }
        }

    private:

        RpcReceiver() = default;

        static void HandleCreate(PeerConnection& peer, uint64_t callId, const std::string& name, NetworkId parentId)
        {
            const auto gameObject = GameObject::Create(IndexedString(name));

            if (GameObjectManager::GetInstance().Has(parentId))
                GameObjectManager::GetInstance().Get(parentId).value()->AddChild(gameObject);

            GameObjectManager::GetInstance().Register(gameObject);

            Settings::GetInstance().REPLICATION_SENDER.Get()->QueueSpawn(gameObject);

            std::ostringstream innerStream;

            {
                cereal::BinaryOutputArchive archive(innerStream);

                archive(static_cast<uint32_t>(1));

                archive(callId, RpcType::CreateGameObjectResponse, gameObject->GetNetworkId(), gameObject->GetName().operator std::string(), gameObject->GetParent().has_value() ? gameObject->GetParent().value()->GetNetworkId() : 0);
            }

            std::ostringstream stream;
            cereal::BinaryOutputArchive archive(stream);

            archive(std::string(RpcClient::RpcChannelName));

            archive(innerStream.str());

            auto const& buffer = stream.str();
            const auto message = Message::Create(Message::Type::Custom, buffer.data(), buffer.size(), true);

            peer.Send(message);

            std::cout << "Sent message with buffer '" << buffer << "' to peer '" << peer.GetHandle() << "' with callId '" << callId << "'." << std::endl;
        }

        static void HandleDestroy(const NetworkId id)
        {
            if (auto optionalGameObject = GameObjectManager::GetInstance().Get(id))
                GameObjectManager::GetInstance().Unregister(id);
        }

        static void HandleAddChild(const NetworkId parentId, const NetworkId childId)
        {
            auto& gameObjectManagerInstance = GameObjectManager::GetInstance();

            if (const auto parent = gameObjectManagerInstance.Get(parentId))
            {
                if (const auto child = gameObjectManagerInstance.Get(childId))
                    parent.value()->AddChild(child.value());
            }
        }

        static void HandleRemoveChild(const NetworkId parentId, const NetworkId childId)
        {
            auto& gameObjectManagerInstance = GameObjectManager::GetInstance();

            if (const auto parent = gameObjectManagerInstance.Get(parentId))
                parent.value()->RemoveChild(childId);
        }

        static void HandleAddComponent(PeerConnection& peer, uint64_t callId, const std::string& compTypeName, NetworkId objectId, const std::string& payload)
        {
            auto optionalGameObject = GameObjectManager::GetInstance().Get(objectId);

            if (!optionalGameObject)
                return;

            auto component = ComponentFactory::Create(compTypeName);
            
            component = optionalGameObject.value()->AddComponentDynamic(component);

            if (auto networkComponent = dynamic_cast<INetworkSerializable*>(component.get()))
            {
                std::istringstream stream(payload);

                cereal::BinaryInputArchive archive(stream);

                networkComponent->Deserialize(archive);
            }

            Settings::GetInstance().REPLICATION_SENDER.Get()->QueueAddComponent(objectId, compTypeName);

            std::ostringstream inner;

            {
                cereal::BinaryOutputArchive archive(inner);

                archive(static_cast<uint32_t>(1));
                archive(callId, RpcType::AddComponentResponse, objectId, compTypeName, payload);
            }

            std::ostringstream outer;

            {
                cereal::BinaryOutputArchive archive(outer);

                archive(std::string(RpcClient::RpcChannelName));
                archive(inner.str());
            }

            auto buffer = outer.str();

            peer.Send(Message::Create(Message::Type::Custom, buffer.data(), buffer.size(), true));
        }

        static void HandleRemoveComponent(const PeerConnection& peer, uint64_t, const std::string& compTypeName, NetworkId objectId)
        {
            if (const auto optionalGameObject = GameObjectManager::GetInstance().Get(objectId))
                optionalGameObject.value()->RemoveComponentDynamic(ComponentFactory::Create(compTypeName));

            Settings::GetInstance().REPLICATION_SENDER.Get()
                    ->QueueRemoveComponent(objectId, compTypeName);

            std::ostringstream inner;

            cereal::BinaryOutputArchive innerArchive(inner);

            innerArchive(static_cast<uint32_t>(1));
            innerArchive(static_cast<uint64_t>(0), RpcType::RemoveComponentResponse, objectId, compTypeName);

            std::ostringstream os;
            cereal::BinaryOutputArchive archive(os);

            archive(std::string(RpcClient::RpcChannelName));
            archive(inner.str());

            peer.Send(Message::Create(Message::Type::Custom, os.str().data(), os.str().size(), true));
        }

        static void BroadcastTransformUpdate(NetworkId id, const Vector<float, 3>& p, const Vector<float, 3>& q)
        {
            std::ostringstream innerStream;

            cereal::BinaryOutputArchive archiveInner(innerStream);

            archiveInner(static_cast<uint32_t>(1), static_cast<uint64_t>(0), RpcType::MoveGameObjectResponse, id, SerializePositionRotation(p,q));

            std::ostringstream outerStream;

            cereal::BinaryOutputArchive archiveOuter(outerStream);

            archiveOuter(std::string(RpcClient::RpcChannelName), innerStream.str());

            auto buffer = outerStream.str();
            auto message = Message::Create(Message::Type::Custom, buffer.data(), buffer.size(), true);

            NetworkManager::GetInstance().Broadcast(message);
        }

        static std::string SerializePositionRotation(const Vector<float, 3>& position, const Vector<float, 3>& rotation)
        {
            std::ostringstream stream;

            cereal::BinaryOutputArchive archive(stream);

            archive(position, rotation);

            return stream.str();
        }

        static std::pair<Vector<float, 3>, Vector<float, 3>> DeserializePositionRotation(const std::string& payload)
        {
            std::istringstream stream(payload);

            cereal::BinaryInputArchive archive(stream);

            Vector<float, 3> position = { 0, 0, 0 };
            Vector<float, 3> rotation = { 0, 0, 0 };

            archive(position, rotation);

            return {position,rotation};
        }
    };
}