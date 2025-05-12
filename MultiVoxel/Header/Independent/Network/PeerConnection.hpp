#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <steam/steamnetworkingsockets.h>
#include "Independent/Network/Message.hpp"

namespace MultiVoxel::Independent::Network
{
    class PeerConnection
    {

    public:

        ~PeerConnection()
        {
            if (connection != k_HSteamNetConnection_Invalid)
                sockets->CloseConnection(connection, 0, nullptr, false);
        }

        [[nodiscard]]
        HSteamNetConnection GetHandle() const
        {
            return connection;
        }

        void Send(const Message& msg) const
        {
            std::vector<uint8_t> buf;

            msg.Serialize(buf);

            sockets->SendMessageToConnection(connection, buf.data(), buf.size(), msg.IsReliable() ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
        }

        void EnqueueReceived(SteamNetworkingMessage_t* incoming)  
        {  
            const auto* data = static_cast<const uint8_t*>(incoming->m_pData);

            if (const size_t bytes = incoming->m_cbSize; bytes >= 1)
            {
                const auto type = static_cast<Message::Type>(data[0]);

                const std::vector payload(data + 1, data + bytes);
                std::lock_guard lock(mutex);

                messageQueue.emplace(Message::Create(type, payload.data(), payload.size(), true));  
            }  

            incoming->Release();
        }

        bool Receive(Message& outMsg)
        {
            std::lock_guard lock(mutex);

            if (messageQueue.empty())
                return false;

            outMsg = std::move(messageQueue.front());
            messageQueue.pop();

            return true;
        }

        static std::unique_ptr<PeerConnection> Create(const HSteamNetConnection connection, ISteamNetworkingSockets* sockets, const HSteamNetPollGroup pollGroup)
        {
            auto result = std::unique_ptr<PeerConnection>(new PeerConnection());

            result->connection = connection;
            result->sockets = sockets;

            sockets->SetConnectionPollGroup(connection, pollGroup);

            return result;
        }

    private:

        PeerConnection() = default;

        HSteamNetConnection connection = 0;
        ISteamNetworkingSockets* sockets = nullptr;
        std::mutex mutex;
        std::queue<Message> messageQueue;
    };
}