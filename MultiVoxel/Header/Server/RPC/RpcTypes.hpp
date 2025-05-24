#pragma once

#include <cstdint>

namespace MultiVoxel::Server::Rpc
{
    enum class RpcType : uint8_t
    {
        CreateGameObject = 0,
        DestroyGameObject = 1,
        AddChild = 2,
        RemoveChild = 3,
        RequestFullSync = 4,
        AddComponent = 5,
        RemoveComponent = 6,
        MoveGameObjectRequest = 7,
        CreateGameObjectResponse = 128,
        AddChildResponse = 129,
        AddComponentResponse = 130,
        RemoveComponentResponse = 131,
        MoveGameObjectResponse = 132
    };

    inline bool RpcNeedsElevation(const RpcType type)
    {
        switch (type)
        {

        case RpcType::CreateGameObject:
        case RpcType::DestroyGameObject:
        case RpcType::AddChild:
        case RpcType::RemoveChild:
        case RpcType::AddComponent:
        case RpcType::RemoveComponent:
            return true;

        default:
            return false;
        }
    }
}