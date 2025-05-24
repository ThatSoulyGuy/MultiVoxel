#pragma once

#include <cereal/archives/binary.hpp>
#include "Client/Core/InputManager.hpp"
#include "Client/Packet/RpcClient.hpp"
#include "Client/Render/Mesh.hpp"
#include "Client/Render/ShaderManager.hpp"
#include "Client/Render/Vertices/FatVertex.hpp"
#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObject.hpp"
#include "Server/Entity/EntityBase.hpp"

using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Client::Packet;
using namespace MultiVoxel::Client::Render::Vertices;
using namespace MultiVoxel::Client::Render;
using namespace MultiVoxel::Independent::Core;

namespace MultiVoxel::Server::Entity::Entities
{
    class EntityPlayer final : public EntityBase
    {

    public:

        EntityPlayer(const EntityPlayer&) = delete;
        EntityPlayer(EntityPlayer&&) = delete;
        EntityPlayer& operator=(const EntityPlayer&) = delete;
        EntityPlayer& operator=(EntityPlayer&&) = delete;

        void Initialize() override
        {
            if (!GetGameObject()->IsAuthoritative())
                return;

            const auto gameObject = GameObject::Create(std::format("default.player_mesh_{}", GetGameObject()->GetNetworkId()));

            RpcClient::GetInstance().AddComponentAsync(gameObject->GetNetworkId(), ShaderManager::GetInstance().Get({ "multivoxel.fat" }).value());
            auto component = RpcClient::GetInstance().AddComponentAsync(gameObject->GetNetworkId(), Mesh<FatVertex>::Create(
            {
                    { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
                    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                    { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } }
            },
            {
                0, 1, 2,
                2, 1, 3
            }));

            Settings::GetInstance().REPLICATION_SENDER.Get()->QueueSpawn(gameObject);
            Settings::GetInstance().REPLICATION_SENDER.Get()->QueueAddChild(GetGameObject()->GetNetworkId(), gameObject->GetNetworkId());

            GetGameObject()->AddChild(gameObject);
        }

        void Update() override
        {
            UpdateMovement();
        }

        void UpdateMovement() const
        {
            if (GetGameObject()->IsAuthoritative())
                return;

            Vector<float, 3> movement = { 0.0f, 0.0f, 0.0f };

            if (InputManager::GetInstance().GetKeyState(KeyCode::W, KeyState::PRESSED))
                movement += { 0.0f, 0.0f, GetMovementSpeed() };

            if (InputManager::GetInstance().GetKeyState(KeyCode::S, KeyState::PRESSED))
                movement -= { 0.0f, 0.0f, GetMovementSpeed() };

            if (InputManager::GetInstance().GetKeyState(KeyCode::A, KeyState::PRESSED))
                movement -= { GetMovementSpeed(), 0.0f, 0.0f };

            if (InputManager::GetInstance().GetKeyState(KeyCode::D, KeyState::PRESSED))
                movement += { GetMovementSpeed(), 0.0f, 0.0f };

            if (movement != Vector<float, 3>{ 0.0f, 0.0f, 0.0f })
                std::cout << movement << std::endl;

            GetGameObject()->GetTransform()->Translate(movement);

            RpcClient::GetInstance().MoveGameObject(GetGameObject()->GetNetworkId(), movement, { 0.0f, 0.0f, 0.0f });
        }

        [[nodiscard]]
        float GetMovementSpeed() const override
        {
            return 0.8f;
        }

        [[nodiscard]]
        float GetRunningMultiplier() const override
        {
            return 1.2f;
        }

        [[nodiscard]]
        float GetMaximumHealth() const override
        {
            return 100;
        }

        void Serialize(cereal::BinaryOutputArchive& archive) const override
        {
            archive(GetCurrentHealth());
        }

        void Deserialize(cereal::BinaryInputArchive& archive) override
        {
            archive(GetCurrentHealth());

            dirty = false;
        }

        void MarkDirty() override
        {
            dirty = true;
        }

        [[nodiscard]]
        bool IsDirty() const override
        {
            return dirty;
        }

        void ClearDirty() override
        {
            dirty = false;
        }

        [[nodiscard]]
        std::string GetComponentTypeName() const override
        {
            return typeid(EntityPlayer).name();
        }

        static std::shared_ptr<EntityPlayer> Create()
        {
            return std::shared_ptr<EntityPlayer>(new EntityPlayer());
        }

    private:

        EntityPlayer() = default;

        friend class MultiVoxel::Independent::ECS::ComponentFactory;

        bool dirty = true;

    };

    REGISTER_COMPONENT(EntityPlayer)
}