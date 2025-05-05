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
#include "Independent/Application.hpp"

using namespace std::chrono;
using namespace MultiVoxel::Independent::Network;
using namespace MultiVoxel::Independent;

namespace MultiVoxel::Client
{
    class ClientBase final
    {

    public:

        void Run()
        {
            if (!isInitialized)
                return;

            const auto frameCap = milliseconds(16);

            while (Application::GetInstance().IsRunning())
            {
                auto start = steady_clock::now();

                auto& networkManager = NetworkManager::GetInstance();

                networkManager.PollEvents();

                for (auto* sender : packetSenderList)
                {
                    IndexedString pktName;
                    std::string pktData;

                    while (sender->SendPacket(pktName, pktData))
                    {
                        std::ostringstream os;
                        cereal::BinaryOutputArchive oa(os);

                        oa(pktName, pktData);

                        auto const& buf = os.str();

                        Message out = Message::Create(
                            Message::Type::Custom,
                            buf.data(),
                            buf.size(),
                            true
                        );

                        networkManager.Broadcast(out);
                    }
                }

                networkManager.FlushOutgoing();

                Application::GetInstance().Update();
                Application::GetInstance().Render();

                auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);

                if (elapsed < frameCap)
                    std::this_thread::sleep_for(frameCap - elapsed);
            }

            Application::GetInstance().Uninitialize();
        }

        void RegisterPacketSender(PacketSender* sender)
        {
            packetSenderList.push_back(sender);
        }

        void RegisterPacketReceiver(PacketReceiver* receiver)
        {
            packetReceiverList.push_back(receiver);
        }

        static std::unique_ptr<ClientBase> Create(const std::string& address, uint16_t port)
        {
            auto result = std::unique_ptr<ClientBase>(new ClientBase());

            result->serverAddress = address;
            result->serverPort = port;
            result->isInitialized = false;

            return (result->Initialize() ? std::move(result) : nullptr);
        }

    private:

        ClientBase() = default;

        bool Initialize()
        {
            auto& networkManager = NetworkManager::GetInstance();

            if (!networkManager.ConnectToServer(serverAddress, serverPort))
            {
                std::cerr << "Client: failed to connect to " << serverAddress << ":" << serverPort << "\n";
                return false;
            }

            networkManager.GetDispatcher()
                .RegisterHandler(Message::Type::Custom,
                    [&](PeerConnection&, Message const& msg)
                    {
                        const auto& buffer = msg.GetBuffer();

                        std::string rawData { reinterpret_cast<const char*>(buffer.data()), buffer.size() };

                        std::istringstream is(rawData);
                        cereal::BinaryInputArchive archive(is);

                        IndexedString name;
                        std::string data;
                        archive(name, data);

                        for (auto* receiver : packetReceiverList)
                            receiver->OnPacketReceived(name, data);
                    });

            Application::GetInstance().Preinitialize();
            Application::GetInstance().Initialize();

            isInitialized = true;

            return true;
        }

        std::string serverAddress;

        uint16_t serverPort;

        bool isInitialized;

        std::vector<PacketSender*> packetSenderList;
        std::vector<PacketReceiver*> packetReceiverList;
    };
}