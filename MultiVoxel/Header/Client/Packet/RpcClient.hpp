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

        bool SendPacket(std::string& outName, std::string& outData) override
        {
            if (requestList.empty())
                return false;

            std::ostringstream os;
            cereal::BinaryOutputArchive ar(os);

            ar(uint32_t(requestList.size()));

            for (auto& r : requestList)
            {
                ar(r.CallId, r.Type);
                ar(r.Name, r.ParentId);
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

                    ar(newId);

                    auto optGO = GameObjectManager::GetInstance().Get(newId);
                    std::shared_ptr<GameObject> ptr = optGO ? *optGO : nullptr;

                    std::lock_guard lk(mutex);

                    auto it = pendingCreateMap.find(callId);

                    if (it != pendingCreateMap.end())
                    {
                        it->second.set_value(ptr);
                        pendingCreateMap.erase(it);
                    }
                }
                // … handle other response types …
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
            uint64_t CallId;
            RpcType  Type;
            std::string Name;
            NetworkId  ParentId;
        };

        uint64_t nextCallId{ 1 };
        std::mutex mutex;
        std::vector<Request> requestList;
        std::unordered_map<uint64_t, std::promise<std::shared_ptr<GameObject>>> pendingCreateMap;
    };

    const std::string RpcClient::RpcChannelName("RpcChannel");
}