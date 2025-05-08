#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include "Client/ClientApplication.hpp"
#include "Independent/Network/NetworkManager.hpp"
#include "Independent/Network/PacketReceiver.hpp"
#include "Independent/Network/PacketSender.hpp"

using namespace std::chrono;
using namespace MultiVoxel::Independent::Network;
using namespace MultiVoxel::Independent;

namespace MultiVoxel::Client
{
    class ClientBase final
    {

    public:

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

                        std::string rawData{ reinterpret_cast<const char*>(buffer.data()), buffer.size() };

                        std::istringstream is(rawData);
                        cereal::BinaryInputArchive archive(is);

                        std::string name;
                        std::string data;
                        archive(name, data);

                        for (auto* receiver : packetReceiverList)
                            receiver->OnPacketReceived(IndexedString(name), data);
                    });

            ClientApplication::GetInstance().Preinitialize();
            ClientApplication::GetInstance().Initialize();

            isInitialized = true;

            return true;
        }

        void Run()
        {
            if (!isInitialized)
                return;

            const auto frameCap = milliseconds(16);

            while (ClientApplication::GetInstance().IsRunning())
            {
                auto start = steady_clock::now();

                auto& networkManager = NetworkManager::GetInstance();

                networkManager.PollEvents();

                for (auto* sender : packetSenderList)
                {
                    std::string pktName;
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

                ClientApplication::GetInstance().Update();
                ClientApplication::GetInstance().Render();

                auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);

                if (elapsed < frameCap)
                    std::this_thread::sleep_for(frameCap - elapsed);
            }

            ClientApplication::GetInstance().Uninitialize();
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