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
    class RpcClient : public PacketSender, public PacketReceiver
    {

    public:

        void Reload() override { }

        std::future<std::shared_ptr<GameObject>> CreateGameObjectAsync(const std::string& name, NetworkId parentId)
        {
            uint64_t callId = nextCallId++;
            {
                std::lock_guard lk(mutex);
                pendingCreateMap.emplace(callId, std::promise<std::shared_ptr<GameObject>>());
            }
            
            requestList.push_back({ callId, RpcType::CreateGameObject, name, parentId });

            return pendingCreateMap[callId].get_future();
        }

        void DestroyGameObject(const std::string& name)
        {
            requestList.push_back({ 0, RpcType::DestroyGameObject, name, 0 });
        }

        void AddChildToGameObject(const std::string& name, NetworkId childId)
        {
            requestList.push_back({ 0, RpcType::AddChild, name, childId });
        }

        void RemoveChildFromGameObject(const std::string& name, NetworkId childId)
        {
            requestList.push_back({ 0, RpcType::RemoveChild, name, childId });
        }

        bool SendPacket(std::string& outName, std::string& outData) override
        {
            if (requestList.empty())
                return false;

            std::ostringstream os;
            cereal::BinaryOutputArchive ar(os);

            ar(uint32_t(requestList.size()));

            for (auto& r : requestList)
            {
                ar(r.callId, r.type);
                ar(r.name, r.parentId);
            }

            requestList.clear();

            outName = RpcChannelName;
            outData = os.str();

            return true;
        }

        void OnPacketReceived(const std::string& name, const std::string& data) override
        {
            if (name != RpcChannelName)
                return;

            std::istringstream is(data);
            cereal::BinaryInputArchive ar(is);

            uint32_t cnt; ar(cnt);

            while (cnt--)
            {
                uint64_t callId;

                RpcType t;

                ar(callId, t);

                if (t == RpcType::CreateGameObjectResponse)
                {
                    NetworkId newId;
                    std::string name;
                    NetworkId parentId;
                    ar(newId, name, parentId);

                    auto go = GameObject::Create(IndexedString(name));

                    go->SetNetworkId(newId);

                    if (parentId != 0)
                        GameObjectManager::GetInstance().Get(parentId).value()->AddChild(go);

                    GameObjectManager::GetInstance().Register(go);

                    std::lock_guard lk(mutex);

                    pendingCreateMap[callId].set_value(go);
                    pendingCreateMap.erase(callId);
                }
            }
        }

        static RpcClient& GetInstance()
        {
            static RpcClient inst;
            return inst;
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
        };

        uint64_t nextCallId{ 1 };
        std::mutex mutex;
        std::vector<Request> requestList;
        std::unordered_map<uint64_t, std::promise<std::shared_ptr<GameObject>>> pendingCreateMap;
    };

    const std::string RpcClient::RpcChannelName("RpcChannel");
}