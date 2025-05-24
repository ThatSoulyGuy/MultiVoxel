#pragma once

#include "Client/Core/Window.hpp"
#include "Independent/ECS/Component.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/ECS/GameObject.hpp"
#include "Independent/Math/Matrix.hpp"

using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Independent::Math
{
    class Camera final : public Component, public INetworkSerializable
    {

    public:

        [[nodiscard]]
        Matrix<float, 4, 4> GetProjectionMatrix() const
        {
            return Matrix<float, 4, 4>::Perspective(fieldOfView * (static_cast<float>(std::numbers::pi) / 180), static_cast<float>(Window::GetInstance().GetDimensions().x()) / static_cast<float>(Window::GetInstance().GetDimensions().y()), nearPlane, farPlane);
        }

        [[nodiscard]]
        Matrix<float, 4, 4> GetViewMatrix() const
        {
            return Matrix<float, 4, 4>::LookAt(GetGameObject()->GetTransform()->GetWorldPosition(), GetGameObject()->GetTransform()->GetWorldPosition() + GetGameObject()->GetTransform()->GetForward(), { 0.0f, 1.0f, 0.0f });
        }

        void Serialize(cereal::BinaryOutputArchive& archive) const override
        {
            archive(fieldOfView, nearPlane, farPlane);
        }

        void Deserialize(cereal::BinaryInputArchive& archive) override
        {
            archive(fieldOfView, nearPlane, farPlane);

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
            return typeid(Camera).name();
        }

        static std::shared_ptr<Camera> Create(const float fieldOfView, const float nearPlane, const float farPlane)
        {
            std::shared_ptr<Camera> result(new Camera());

            result->fieldOfView = fieldOfView;
            result->nearPlane = nearPlane;
            result->farPlane = farPlane;

            return result;
        }

    private:

        Camera() = default;

        friend class MultiVoxel::Independent::ECS::ComponentFactory;

        bool dirty = true;

        float fieldOfView = 0;
        float nearPlane = 0;
        float farPlane = 0;

    };

    REGISTER_COMPONENT(Camera)
}