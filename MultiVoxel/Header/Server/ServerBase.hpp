#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include "Independent/Core/Settings.hpp"
#include "Independent/Network/NetworkManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PacketSender.hpp"
#include "Server/Packet/RpcReceiver.hpp"
#include "Server/ServerInterfaceLayer.hpp"

using namespace std::chrono;
using namespace MultiVoxel::Independent::Core;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Server
{
    class ServerBase final
    {

    public:

        ServerBase(const ServerBase&) = delete;
        ServerBase(ServerBase&&) = delete;
        ServerBase& operator=(const ServerBase&) = delete;
        ServerBase& operator=(ServerBase&&) = delete;

        bool Initialize(const uint32_t port)
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
                    [&](PeerConnection& peer, Message const& msg)
                    {
                        const auto& buffer = msg.GetBuffer();

                        const std::string rawData{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };

                        std::istringstream is(rawData);

                        cereal::BinaryInputArchive archive(is);

                        std::string name;
                        std::string data;

                        archive(name, data);

                        if (name == "RpcChannel")
                            RpcReceiver::HandleRpc(peer, data);

                        for (auto* receiver : packetReceiverList)
                            receiver->OnPacketReceived(name, data);
                    });

            networkManager.AddOnPlayerConnectedCallback([&]()
            {
                for (auto* sender : packetSenderList)
                    sender->Reload();
            });

            ServerInterfaceLayer::GetInstance().CallEvent("preinitialize");
            ServerInterfaceLayer::GetInstance().CallEvent("initialize");
            
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
                        std::ostringstream stream;
                        cereal::BinaryOutputArchive archive(stream);

                        archive(packetName, packetData);
                        auto const& buffer = stream.str();

                        networkManager.Broadcast(Message::Create(Message::Type::Custom, buffer.data(), buffer.size(), true));
                    }
                }

                networkManager.FlushOutgoing();

                ServerInterfaceLayer::GetInstance().CallEvent("update");
                ServerInterfaceLayer::GetInstance().CallEvent("render");

                if (auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start); elapsed < tickInterval)
                    std::this_thread::sleep_for(tickInterval - elapsed);
            }
        }

        void Stop()
        {
            ServerInterfaceLayer::GetInstance().CallEvent("uninitialize");

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

        std::unordered_map<std::string, HSteamNetConnection> players = {};

        std::vector<PacketSender*> packetSenderList;
        std::vector<PacketReceiver*> packetReceiverList;

        static std::once_flag initializationFlag;
        static std::unique_ptr<ServerBase> instance;

    };

    std::once_flag ServerBase::initializationFlag;
    std::unique_ptr<ServerBase> ServerBase::instance;
}