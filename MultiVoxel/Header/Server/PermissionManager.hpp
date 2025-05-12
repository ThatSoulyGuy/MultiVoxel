#pragma once

#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <steam/steamnetworkingsockets.h>
#include "Server/RPC/RpcTypes.hpp"

namespace MultiVoxel::Server
{
    class PermissionManager final
    {

    public:

        PermissionManager(const PermissionManager&) = delete;
        PermissionManager(PermissionManager&&) = delete;
        PermissionManager& operator=(const PermissionManager&) = delete;
        PermissionManager& operator=(PermissionManager&&) = delete;

        void GrantPermission(const HSteamNetConnection peer, const RpcType rpc)
        {
            std::lock_guard guard{ mutex };

            permissions[peer].insert(rpc);
        }

        void RevokePermission(const HSteamNetConnection peer, const RpcType rpc)
        {
            std::lock_guard guard{ mutex };

            permissions[peer].erase(rpc);
        }

        bool HasPermission(const HSteamNetConnection peer, const RpcType rpc) const
        {
            if (!RpcNeedsElevation(rpc))
                return true;

            std::lock_guard guard{ mutex };

            const auto iterator = permissions.find(peer);

            return iterator != permissions.end() && iterator->second.contains(rpc);
        }

        static PermissionManager& GetInstance()
        {
            std::call_once(initializationFlag, [&]()
            {
                instance = std::unique_ptr<PermissionManager>(new PermissionManager());
            });

            return *instance;
        }

    private:

        PermissionManager() = default;

        std::unordered_map<HSteamNetConnection, std::unordered_set<RpcType>> permissions;

        mutable std::mutex mutex;
        
        static std::once_flag initializationFlag;
        static std::unique_ptr<PermissionManager> instance;

    };

    std::once_flag PermissionManager::initializationFlag;
    std::unique_ptr<PermissionManager> PermissionManager::instance;
}