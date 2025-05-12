#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingtypes.h>
#include "Independent/Network/Message.hpp"
#include "Independent/Network/MessageDispatcher.hpp"
#include "Independent/Network/PeerConnection.hpp"

namespace MultiVoxel::Independent::Network
{
    class NetworkManager
    {

    public:

        ~NetworkManager()
        {
            Shutdown();
            GameNetworkingSockets_Kill();
        }

        void PollEvents()  
        {
            sockets->RunCallbacks();

            SteamNetworkingMessage_t* netMsg = nullptr;

            while (sockets->ReceiveMessagesOnPollGroup(pollGroup, &netMsg, 1) > 0)
            {
                for (auto& peer : peerList)
                {
                    if (peer->GetHandle() == netMsg->m_conn)
                    {
                        peer->EnqueueReceived(netMsg);
                        break;
                    }
                }
            }
            
            for (auto& peer : peerList)
            {
                Message msg;

                while (peer->Receive(msg))
                    messageDispatcher.Dispatch(*peer, msg);
            }
        }

        void Broadcast(const Message& message) const
        {
            for (auto& peer : peerList)
                peer->Send(message);
        }

        void FlushOutgoing() const
        {
            sockets->RunCallbacks();
        }

        bool StartServer(const uint16_t port)
        {
            SteamNetworkingIPAddr address = { };

            address.Clear();
            address.SetIPv4(0, port);

            SteamNetworkingConfigValue_t opt = {};

            opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChanged);

            listenerSocket = sockets->CreateListenSocketIP(address, 1, &opt);

            return listenerSocket != k_HSteamListenSocket_Invalid;
        }

        bool ConnectToServer(const std::string& addressString, uint16_t port)
        {
            SteamNetworkingIPAddr address = { };

            SteamNetworkingIPAddr_ParseString(&address, addressString.c_str());

            address.m_port = port;

            SteamNetworkingConfigValue_t opt = {};

            opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChanged);

            const HSteamNetConnection connection = sockets->ConnectByIPAddress(address, 1, &opt);

            if (connection == k_HSteamNetConnection_Invalid)
                return false;

            sockets->SetConnectionPollGroup(connection, pollGroup);

            peerList.push_back(PeerConnection::Create(connection, sockets, pollGroup));

            return true;
        }

        MessageDispatcher& GetDispatcher()
        {
            return messageDispatcher;
        }

        ISteamNetworkingSockets* GetSockets() const
        {
            return sockets;
        }

        HSteamNetPollGroup& GetPollGroup()
        {
            return pollGroup;
        }

        void AddOnPlayerConnectedCallback(const std::function<void()>& callback)
        {
            playerConnectedCallbackList.push_back(callback);
        }

        static NetworkManager& GetInstance()
        {
            std::call_once(initializationFlag, [&]()
            {
                instance = [&]() -> std::unique_ptr<NetworkManager>
                {
                    auto result = std::unique_ptr<NetworkManager>(new NetworkManager());

                    SteamDatagramErrMsg errorMessage;

                    if (!GameNetworkingSockets_Init(nullptr, errorMessage))
                    {
                        std::cerr << "GNS_Init failed: " << errorMessage << "\n";

                        return nullptr;
                    }

                    if (!result->Initialize())
                    {
                        result->Shutdown();

                        return nullptr;
                    }

                    return result;
                }();
            });

            return *instance;
        }

    private:

        NetworkManager() = default;

        bool Initialize()
        {
            sockets = SteamNetworkingSockets();
            utilities = SteamNetworkingUtils();
            pollGroup = sockets->CreatePollGroup();
            
            utilities->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Verbose, &NetworkManager::OnDebugOutput);

            return pollGroup != 0;
        }

        void Shutdown()
        {
            if (listenerSocket != k_HSteamListenSocket_Invalid)
            {
                sockets->CloseListenSocket(listenerSocket);
                listenerSocket = k_HSteamListenSocket_Invalid;
            }

            if (pollGroup)
            {
                sockets->DestroyPollGroup(pollGroup);
                pollGroup = 0;
            }

            peerList.clear();
        }

        static void OnDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* message)
        {
            std::lock_guard lock(logMutex);

            switch (eType)
            {
                case k_ESteamNetworkingSocketsDebugOutputType_None:      break;
                case k_ESteamNetworkingSocketsDebugOutputType_Bug:       std::cerr << "[GNS][BUG] ";       break;
                case k_ESteamNetworkingSocketsDebugOutputType_Error:     std::cerr << "[GNS][ERROR] ";     break;
                case k_ESteamNetworkingSocketsDebugOutputType_Important: std::cerr << "[GNS][IMPORTANT] "; break;
                case k_ESteamNetworkingSocketsDebugOutputType_Warning:   std::cerr << "[GNS][WARNING] ";   break;
                case k_ESteamNetworkingSocketsDebugOutputType_Msg:       std::cerr << "[GNS][INFO] ";      break;
                case k_ESteamNetworkingSocketsDebugOutputType_Verbose:   std::cerr << "[GNS][DEBUG] ";     break;

                default:
                    break;
            }

            std::cerr << message << "\n";
        }
        
        static void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
        {
            if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting && info->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid)
            {
                instance->sockets->AcceptConnection(info->m_hConn);
                instance->sockets->SetConnectionPollGroup(info->m_hConn, instance->pollGroup);
                instance->peerList.push_back(PeerConnection::Create(info->m_hConn, instance->sockets, instance->pollGroup));

                std::cout << "New client connected (handle=" << info->m_hConn << ")\n";
            }
            else switch (info->m_info.m_eState)
            {

            case k_ESteamNetworkingConnectionState_Connected:
                std::cout << "Connection fully established: (handle=" << info->m_hConn << ")\n";

                for (auto& callback : instance->playerConnectedCallbackList)
                    callback();

                break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
                std::erase_if(
                    instance->peerList,
                    [&](auto& p) { return p->GetHandle() == info->m_hConn; }
                );
                std::cout << "Client disconnected (handle=" << info->m_hConn << ")\n";
                break;

            default:
                break;
            }
        }

        static std::mutex logMutex;

        ISteamNetworkingSockets* sockets = nullptr;
        ISteamNetworkingUtils* utilities = nullptr;
        HSteamNetPollGroup pollGroup = 0;
        HSteamListenSocket listenerSocket = k_HSteamListenSocket_Invalid;

        std::vector<std::function<void()>> playerConnectedCallbackList;
        std::vector<std::unique_ptr<PeerConnection>> peerList;

        MessageDispatcher messageDispatcher;

        static std::once_flag initializationFlag;
        static std::unique_ptr<NetworkManager> instance;

    };
    
    std::mutex NetworkManager::logMutex;

    std::once_flag NetworkManager::initializationFlag;
    std::unique_ptr<NetworkManager> NetworkManager::instance;
}