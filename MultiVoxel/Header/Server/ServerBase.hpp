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

using namespace std::chrono;
using namespace MultiVoxel::Independent::Network;

namespace MultiVoxel::Server
{
    class ServerBase final
    {

    public:

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
                    IndexedString packetName;
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

                for (auto& callback : onUpdateCallbackList)
                    callback();

                auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);

                if (elapsed < tickInterval)
                    std::this_thread::sleep_for(tickInterval - elapsed);
            }
        }

        void Stop()
        {
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

        void RegisterOnUpdateCallback(const std::function<void()> callback)
        {
            onUpdateCallbackList.push_back(callback);
        }

        static std::unique_ptr<ServerBase> Create(uint16_t port)
        {
            auto result = std::unique_ptr<ServerBase>(new ServerBase());

            result->serverPort = port;
            result->running = false;
            result->isInitialized = false;

            return (result->Initialize() ? std::move(result) : nullptr);
        }

    private:

        ServerBase() = default;

        bool Initialize()
        {
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

            isInitialized = true;
            return true;
        }

        uint16_t serverPort;

        std::atomic<bool> running;

        bool isInitialized;

        std::vector<std::function<void()>> onUpdateCallbackList;
        std::vector<PacketSender*> packetSenderList;
        std::vector<PacketReceiver*> packetReceiverList;
    };
}