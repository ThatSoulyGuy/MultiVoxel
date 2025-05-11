#pragma once

#include <cstdint>
#include <cereal/archives/binary.hpp>

namespace MultiVoxel::Server::Rpc
{
    enum class RpcType : uint8_t
    {
        CreateGameObject = 0,
        DestroyGameObject = 1,
        AddChild = 2,
        RemoveChild = 3,
        RequestFullSync = 4,
        CreateGameObjectResponse = 128,
        AddChildResponse = 129,
    };

    inline bool RpcNeedsElevation(RpcType t)
    {
        switch (t)
        {

        case RpcType::CreateGameObject:
        case RpcType::DestroyGameObject:
        case RpcType::AddChild:
        case RpcType::RemoveChild:
            return true;

        default:
            return false;
        }
    }
}