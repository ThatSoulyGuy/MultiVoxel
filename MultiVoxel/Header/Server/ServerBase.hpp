#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include "Independent/Network/NetworkManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PacketSender.hpp"
#include "Server/ServerApplication.hpp"

using namespace std::chrono;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Server
{
    class ServerBase final
    {

    public:

        bool Initialize(uint32_t port)
        {
            serverPort = port;

            auto& networkManager = NetworkManager::GetInstance();

            if (!networkManager.StartServer(serverPort))
            {
                std::cerr << "Server: failed to bind port " << serverPort << "\n";
                return false;
            }

            networkManager.GetDispatcher()
                .RegisterHandler(Message::Type::Custom,
                    [&](PeerConnection&, Message const& msg)
                    {
                        const auto& buffer = msg.GetBuffer();

                        std::string rawData{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };

                        std::istringstream is(rawData);

                        cereal::BinaryInputArchive archive(is);

                        IndexedString name;

                        std::string data;

                        archive(name, data);

                        for (auto* receiver : packetReceiverList)
                            receiver->OnPacketReceived(name, data);
                    });

            networkManager.AddOnPlayerConnectedCallback([&]()
            {
                Settings::GetInstance().REPLICATION_SENDER.Get().Reset();

                for (auto& gameObject : GameObjectManager::GetInstance().GetAll())
                    Settings::GetInstance().REPLICATION_SENDER.Get().QueueSpawn(gameObject);

                for (auto& go : GameObjectManager::GetInstance().GetAll())
                {
                    for (auto& [ti, comp] : go->GetComponentMap())
                    {
                        if (auto net = dynamic_cast<INetworkSerializable*>(comp.get()))
                            net->MarkDirty();
                    }
                }
            });

            ServerApplication::GetInstance().Preinitialize();
            ServerApplication::GetInstance().Initialize();
            
            isInitialized = true;

            return true;
        }

        void Run()
        {
            if (!isInitialized)
                return;

            running = true;

            const auto tickInterval = milliseconds(50);

            while (running)
            {
                auto start = steady_clock::now();

                auto& networkManager = NetworkManager::GetInstance();

                networkManager.PollEvents();

                for (auto* sender : packetSenderList)
                {
                    std::string packetName;
                    std::string packetData;

                    while (sender->SendPacket(packetName, packetData))
                    {
                        std::ostringstream os;
                        cereal::BinaryOutputArchive oa(os);

                        oa(packetName, packetData);
                        auto const& buffer = os.str();

                        networkManager.Broadcast(Message::Create(Message::Type::Custom, buffer.data(), buffer.size(), true));
                    }
                }

                networkManager.FlushOutgoing();

                ServerApplication::GetInstance().Update();
                ServerApplication::GetInstance().Render();

                auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);

                if (elapsed < tickInterval)
                    std::this_thread::sleep_for(tickInterval - elapsed);
            }
        }

        void Stop()
        {
            ServerApplication::GetInstance().Uninitialize();

            running = false;
        }

        void RegisterPacketSender(PacketSender* sender)
        {
            packetSenderList.push_back(sender);
        }

        void RegisterPacketReceiver(PacketReceiver* receiver)
        {
            packetReceiverList.push_back(receiver);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of<MultiVoxel::Independent::ECS::Component, T>::value>>
        void RegisterECSComponentId(uint32_t id, std::shared_ptr<T> component)
        {
            if (componentIdMap.contains(id))
            {
                std::cerr << "Component ID map already contains id '" << id << "'!" << std::endl;
                return;
            }

            componentIdMap.insert({ id, std::static_pointer_cast<Component>(component) });
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of<MultiVoxel::Independent::ECS::Component, T>::value>>
        std::shared_ptr<T> GetECSComponentById(uint32_t id)
        {
            if (!componentIdMap.contains(id))
            {
                std::cerr << "Component ID map doesn't contain id '" << id << "'!" << std::endl;
                return nullptr;
            }

            return std::static_pointer_cast<T>(componentIdMap[id]);
        }

        void UnregisterECSComponentId(uint32_t id)
        {
            if (!componentIdMap.contains(id))
            {
                std::cerr << "Component ID map doesn't contain id '" << id << "'!" << std::endl;
                return;
            }

            componentIdMap.erase(id);
        }

        static ServerBase& GetInstance()
        {
            std::call_once(initializationFlag, [&]()
            {
                instance = []()
                {
                    auto result = std::unique_ptr<ServerBase>(new ServerBase());

                    result->running = false;
                    result->isInitialized = false;

                    return std::move(result);
                }();
            });
            
            return *instance;
        }

    private:

        ServerBase() = default;

        uint16_t serverPort = 0;

        std::atomic<bool> running = false;

        bool isInitialized = false;

        std::vector<PacketSender*> packetSenderList;
        std::vector<PacketReceiver*> packetReceiverList;

        std::unordered_map<uint32_t, std::weak_ptr<MultiVoxel::Independent::ECS::Component>> componentIdMap;
        
        static std::once_flag initializationFlag;
        static std::unique_ptr<ServerBase> instance;

    };

    std::once_flag ServerBase::initializationFlag;
    std::unique_ptr<ServerBase> ServerBase::instance;
}