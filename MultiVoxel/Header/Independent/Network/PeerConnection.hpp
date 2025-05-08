#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <GameNetworkingSockets/steamnetworkingsockets.h>
#include "Independent/Network/Message.hpp"

namespace MultiVoxel::Independent::Network
{
    class PeerConnection
    {

    public:

        static std::unique_ptr<PeerConnection> Create(HSteamNetConnection conn, ISteamNetworkingSockets* sockets, HSteamNetPollGroup pollGroup)
        {
            auto result = std::unique_ptr<PeerConnection>(new PeerConnection(conn, sockets));

            sockets->SetConnectionPollGroup(conn, pollGroup);

            return result;
        }

        ~PeerConnection()
        {
            if (connection != k_HSteamNetConnection_Invalid)
                sockets->CloseConnection(connection, 0, nullptr, false);
        }

        HSteamNetConnection GetHandle() const 
        {
            return connection;
        }

        void Send(const Message& msg)
        {
            std::vector<uint8_t> buf;

            msg.Serialize(buf);

            sockets->SendMessageToConnection(connection, buf.data(), buf.size(), msg.IsReliable() ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
        }

        void EnqueueReceived(SteamNetworkingMessage_t* incoming)  
        {  
            const uint8_t* data = static_cast<const uint8_t*>(incoming->m_pData);  
            
            size_t bytes = incoming->m_cbSize;  

            if (bytes >= 1)  
            {  
                Message::Type type = static_cast<Message::Type>(data[0]);  

                std::vector<uint8_t> payload(data + 1, data + bytes);  
                std::lock_guard<std::mutex> lock(mutex);  

                messageQueue.emplace(Message::Create(type, payload.data(), payload.size(), true));  
            }  

            incoming->Release();
        }

        bool Receive(Message& outMsg)
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (messageQueue.empty())
                return false;

            outMsg = std::move(messageQueue.front());
            messageQueue.pop();

            return true;
        }

    private:

        PeerConnection(HSteamNetConnection conn, ISteamNetworkingSockets* sockets) : connection(conn), sockets(sockets) { }

        HSteamNetConnection connection;
        ISteamNetworkingSockets* sockets;
        std::mutex mutex;
        std::queue<Message> messageQueue;
    };
}