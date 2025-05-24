#pragma once

#include "Independent/ECS/Component.hpp"
#include "Independent/ECS/ComponentFactory.hpp"

using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Server::Entity
{
    class EntityBase : public Component, public INetworkSerializable
    {

    public:

        [[nodiscard]]
        virtual float GetMovementSpeed() const = 0;

        [[nodiscard]]
        virtual float GetRunningMultiplier() const = 0;

        [[nodiscard]]
        virtual float GetMaximumHealth() const = 0;

        [[nodiscard]]
        float GetCurrentHealth() const
        {
            return currentHealth;
        }

    private:

        float currentHealth = 0;

    };
}