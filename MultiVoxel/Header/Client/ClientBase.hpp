#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "Client/Core/Window.hpp"
#include "Client/ClientInterfaceLayer.hpp"
#include "Independent/Network/NetworkManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PacketSender.hpp"

using namespace std::chrono;
using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Independent::Network;
using namespace MultiVoxel::Independent;

namespace MultiVoxel::Client
{
    class ClientBase final
    {

    public:

        ClientBase(const ClientBase&) = delete;
        ClientBase(ClientBase&&) = delete;
        ClientBase& operator=(const ClientBase&) = delete;
        ClientBase& operator=(ClientBase&&) = delete;

        bool Initialize(const std::string& address, uint32_t port)
        {
            serverAddress = address;
            serverPort = port;

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

                        std::string dataIn{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };

                        std::istringstream stream(dataIn);
                        cereal::BinaryInputArchive archive(stream);

                        std::string name;
                        std::string data;

                        archive(name, data);

                        for (auto* receiver : packetReceiverList)
                            receiver->OnPacketReceived(IndexedString(name), data);
                    });

            ClientInterfaceLayer::GetInstance().CallEvent("preinitialize");
            ClientInterfaceLayer::GetInstance().CallEvent("initialize");

            isInitialized = true;

            return true;
        }

        void Run()
        {
            if (!isInitialized)
                return;

            const auto frameCap = milliseconds(16);

            while (Window::GetInstance().IsRunning())
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

                ClientInterfaceLayer::GetInstance().CallEvent("update");
                ClientInterfaceLayer::GetInstance().CallEvent("render");

                auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);

                if (elapsed < frameCap)
                    std::this_thread::sleep_for(frameCap - elapsed);
            }

            ClientInterfaceLayer::GetInstance().CallEvent("uninitialize");
        }

        void RegisterPacketSender(PacketSender* sender)
        {
            packetSenderList.push_back(sender);
        }

        void RegisterPacketReceiver(PacketReceiver* receiver)
        {
            packetReceiverList.push_back(receiver);
        }

        static ClientBase& GetInstance()
        {
            std::call_once(initializationFlag, [&]()
            {
                instance = []()
                {
                    auto result = std::unique_ptr<ClientBase>(new ClientBase());

                    result->isInitialized = false;

                    return std::move(result);
                }();
            });

            return *instance;
        }

    private:

        ClientBase() = default;

        std::string serverAddress;

        uint16_t serverPort = 0;

        bool isInitialized = false;

        std::vector<PacketSender*> packetSenderList;
        std::vector<PacketReceiver*> packetReceiverList;
        
        static std::once_flag initializationFlag;
        static std::unique_ptr<ClientBase> instance;

    };

    std::once_flag ClientBase::initializationFlag;
    std::unique_ptr<ClientBase> ClientBase::instance;
}