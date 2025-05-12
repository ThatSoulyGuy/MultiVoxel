#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <future>
#include <cereal/archives/binary.hpp>
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PacketSender.hpp"
#include "Server/RPC/RpcTypes.hpp"

using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Network;
using namespace MultiVoxel::Server::Rpc;

namespace MultiVoxel::Client::Packet
{
    class RpcClient final : public PacketSender, public PacketReceiver
    {

    public:

        void Reload() override
        {
            requestList.push_back({ 0, RpcType::RequestFullSync, "", 0});
        }

        std::future<std::shared_ptr<GameObject>> CreateGameObjectAsync(const std::string& name, const NetworkId parentId)
        {
            uint64_t callId = nextCallId++;

            {
                std::lock_guard guard(mutex);
                pendingCreateMap.emplace(callId, std::promise<std::shared_ptr<GameObject>>());
            }
            
            requestList.push_back({ callId, RpcType::CreateGameObject, name, parentId });

            return pendingCreateMap[callId].get_future();
        }

        void DestroyGameObject(const std::string& name)
        {
            requestList.push_back({ 0, RpcType::DestroyGameObject, name, 0 });
        }

        void AddChildToGameObject(const std::string& name, const NetworkId childId)
        {
            requestList.push_back({ 0, RpcType::AddChild, name, childId });
        }

        void RemoveChildFromGameObject(const std::string& name, const NetworkId childId)
        {
            requestList.push_back({ 0, RpcType::RemoveChild, name, childId });
        }

        std::future<std::shared_ptr<Component>> AddComponentAsync(const NetworkId objectId, const std::string& compTypeName, const std::string& payload)
        {
            uint64_t callId = nextCallId++;
            {
                std::lock_guard guard(mutex);

                pendingComponentMap.emplace(callId, std::promise<std::shared_ptr<Component>>());
            }

            requestList.push_back({ callId, RpcType::AddComponent, compTypeName, objectId, payload });

            return pendingComponentMap[callId].get_future();
        }

        template<ComponentType T>
        std::future<std::shared_ptr<T>> AddComponentAsync(const NetworkId objectId, std::shared_ptr<T> prototype)
        {
            static_assert(std::derived_from<T, Component>);

            std::ostringstream stream;
            {
                cereal::BinaryOutputArchive archive(stream);
                prototype->Serialize(archive);
            }

            auto baseFuture = AddComponentAsync(objectId, typeid(T).name(), stream.str());

            std::promise<std::shared_ptr<T>> promise;
            auto result = promise.get_future();

            std::thread([baseFuture = std::move(baseFuture), promise = std::move(promise)]() mutable
            {
                const auto basePointer = baseFuture.get();
                auto derived = std::dynamic_pointer_cast<T>(basePointer);

                promise.set_value(derived);
            }).detach();

            return result;
        }

        void RemoveComponent(const NetworkId objectId, const std::string& typeName)
        {
            requestList.push_back({ 0, RpcType::RemoveComponent, typeName, objectId });
        }

        bool SendPacket(std::string& outName, std::string& outData) override
        {
            if (requestList.empty())
                return false;

            std::ostringstream stream;
            cereal::BinaryOutputArchive archive(stream);

            archive(static_cast<uint32_t>(requestList.size()));

            for (auto& [callId, type, name, parentId, payload] : requestList)
            {
                archive(callId, type);

                switch (type)
                {
                    case RpcType::CreateGameObject:
                    case RpcType::AddChild:
                    case RpcType::RequestFullSync:
                        archive(name, parentId);
                        break;

                    case RpcType::DestroyGameObject:
                        archive(parentId);
                        break;

                    case RpcType::AddComponent:
                        archive(name, parentId);
                        archive(payload);
                        break;

                    case RpcType::RemoveComponent:
                        archive(name, parentId);
                        break;

                    default:
                        break;
                }
            }

            requestList.clear();

            outName = RpcChannelName;
            outData = stream.str();

            return true;
        }

        void OnPacketReceived(const std::string& name, const std::string& data) override
        {
            if (name != RpcChannelName)
                return;

            std::istringstream stream(data);
            cereal::BinaryInputArchive archive(stream);

            uint32_t callCount;
            archive(callCount);

            while (callCount--)
            {
                uint64_t callId;
                RpcType rpcType;

                archive(callId, rpcType);

                if (rpcType == RpcType::CreateGameObjectResponse)
                {
                    NetworkId newId;

                    std::string gameObjectName;

                    NetworkId parentId;

                    archive(newId, gameObjectName, parentId);

                    auto gameObject = GameObject::Create(IndexedString(gameObjectName));

                    gameObject->SetNetworkId(newId);

                    if (parentId != 0)
                        GameObjectManager::GetInstance().Get(parentId).value()->AddChild(gameObject);

                    GameObjectManager::GetInstance().Register(gameObject);

                    std::lock_guard guard(mutex);

                    pendingCreateMap[callId].set_value(gameObject);
                    pendingCreateMap.erase(callId);
                }
                else if (rpcType == RpcType::AddComponentResponse)
                {
                    NetworkId objectId;
                    std::string compTypeName;
                    archive(objectId, compTypeName);

                    std::string payload;

                    archive(payload);

                    auto optionalGameObject = GameObjectManager::GetInstance().Get(objectId);

                    std::shared_ptr<Component> component = nullptr;

                    if (optionalGameObject)
                    {
                        component = ComponentFactory::Create(compTypeName);
                        
                        optionalGameObject.value()->AddComponentDynamic(component);

                        {
                            std::istringstream componentStream(payload);
                            cereal::BinaryInputArchive componentArchive(componentStream);

                            if (auto* networkComponent = dynamic_cast<INetworkSerializable*>(component.get()))
                                networkComponent->Deserialize(componentArchive);
                        }
                    }

                    std::lock_guard guard(mutex);

                    if (auto iterator = pendingComponentMap.find(callId); iterator != pendingComponentMap.end())
                    {
                        iterator->second.set_value(component);

                        pendingComponentMap.erase(iterator);
                    }
                }
                else if (rpcType == RpcType::RemoveComponentResponse)
                {
                    NetworkId objectId;

                    std::string compTypeName;
                    archive(objectId, compTypeName);

                    if (auto optionalGameObject = GameObjectManager::GetInstance().Get(objectId))
                        optionalGameObject.value()->RemoveComponentDynamic(ComponentFactory::Create(compTypeName));
                }
            }
        }

        static RpcClient& GetInstance()
        {
            std::call_once(initializationFlag, [&]()
            {
                instance = std::unique_ptr<RpcClient>(new RpcClient());
            });
            
            return *instance;
        }

        static const std::string RpcChannelName;

    private:

        RpcClient() = default;

        struct Request
        {
            uint64_t callId;
            RpcType type;
            std::string name;
            NetworkId parentId;
            std::string payload;
        };

        uint64_t nextCallId{ 1 };

        std::mutex mutex;
        std::vector<Request> requestList;
        std::unordered_map<uint64_t, std::promise<std::shared_ptr<GameObject>>> pendingCreateMap;
        std::unordered_map<uint64_t, std::promise<std::shared_ptr<Component>>> pendingComponentMap;

        static std::once_flag initializationFlag;
        static std::unique_ptr<RpcClient> instance;

    };
    
    const std::string RpcClient::RpcChannelName("RpcChannel");

    std::once_flag RpcClient::initializationFlag;
    std::unique_ptr<RpcClient> RpcClient::instance;

}