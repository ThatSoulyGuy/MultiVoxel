#pragma once

#include <memory>
#include <optional>
#include <numbers>
#include <functional>
#include <cereal/types/optional.hpp>
#include "Independent/ECS/Component.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/Math/Matrix.hpp"
#include "Independent/Math/Vector.hpp"

using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Independent::Math
{
    class Transform final : public Component, public INetworkSerializable
    {

    public:

        void Translate(const Vector<float, 3>& translation)
        {
            localPosition += translation;

            for (auto& function : onPositionUpdated)
                function(localPosition);

            dirty = true;
        }

        void Rotate(const Vector<float, 3>& rotationDeg)
        {
            localRotation += rotationDeg;

            for (int i = 0; i < 3; ++i)
            {
                localRotation[i] = std::fmod(localRotation[i], 360.0f);

                if (localRotation[i] < 0.0f)
                    localRotation[i] += 360.0f;
            }

            for (auto& function : onRotationUpdated)
                function(localRotation);

            dirty = true;
        }

        void Scale(const Vector<float, 3>& scale)
        {
            localScale += scale;

            for (auto& function : onScaleUpdated)
                function(localScale);

            dirty = true;
        }

        [[nodiscard]]
        Vector<float, 3> GetLocalPosition() const
        {
            return localPosition;
        }

        void SetLocalPosition(const Vector<float, 3>& value, const bool update = true)
        {
            localPosition = value;

            if (!update)
                return;

            for (auto& function : onPositionUpdated)
                function(localPosition);

            dirty = true;
        }

        [[nodiscard]]
        Vector<float, 3> GetLocalRotation() const
        {
            return localRotation;
        }

        void SetLocalRotation(const Vector<float, 3>& value, bool update = true)
        {
            localRotation = value;

            for (int i = 0; i < 3; ++i)
            {
                localRotation[i] = std::fmod(localRotation[i], 360.0f);

                if (localRotation[i] < 0.0f)
                    localRotation[i] += 360.0f;
            }

            if (!update)
                return;

            for (auto& function : onRotationUpdated)
                function(localRotation);

            dirty = true;
        }

        [[nodiscard]]
        Vector<float, 3> GetLocalScale() const
        {
            return localScale;
        }

        void SetLocalScale(const Vector<float, 3>& value, const bool update = true)
        {
            localScale = value;

            if (!update)
                return;

            for (auto& function : onScaleUpdated)
                function(localScale);

            dirty = true;
        }

        [[nodiscard]]
        Vector<float, 3> GetWorldPosition() const
        {
            auto model = GetModelMatrix();

            return { model[3][0], model[3][1], model[3][2] };
        }

        [[nodiscard]]
        Vector<float, 3> GetWorldScale() const
        {
            auto model = GetModelMatrix();

            auto length = [](const float x, const float y, const float z)
            {
                return std::sqrt(x * x + y * y + z * z);
            };

            return
            {
                length(model[0][0], model[0][1], model[0][2]),
                length(model[1][0], model[1][1], model[1][2]),
                length(model[2][0], model[2][1], model[2][2])
            };
        }

        [[nodiscard]]
        Vector<float, 3> GetForward() const
        {
            auto model = GetModelMatrix();

            return Vector<float, 3>::Normalize({ model[2][0], model[2][1], model[2][2] });
        }

        [[nodiscard]]
        Vector<float, 3> GetRight() const
        {
            auto model = GetModelMatrix();

            return Vector<float, 3>::Normalize({ model[0][0], model[0][1], model[0][2] });
        }

        [[nodiscard]]
        Vector<float, 3> GetUp() const
        {
            auto model = GetModelMatrix();

            return Vector<float, 3>::Normalize({ model[1][0], model[1][1], model[1][2] });
        }

        [[nodiscard]]
        Vector<float, 3> GetWorldRotation() const
        {
            auto model = GetModelMatrix();

            Vector<float, 3> sc = GetWorldScale();

            if (std::fabs(sc.x()) > 1e-6f)
                model[0][0] /= sc.x(); model[0][1] /= sc.x(); model[0][2] /= sc.x();

            if (std::fabs(sc.y()) > 1e-6f)
                model[1][0] /= sc.y(); model[1][1] /= sc.y(); model[1][2] /= sc.y();

            if (std::fabs(sc.z()) > 1e-6f)
                model[2][0] /= sc.z(); model[2][1] /= sc.z(); model[2][2] /= sc.z();


            float pitch, yaw, roll;

            if (std::fabs(model[0][0]) < 1e-6f && std::fabs(model[1][0]) < 1e-6f)
            {
                pitch = std::atan2(-model[2][0], model[2][2]);
                yaw = 0.0f;
                roll = std::atan2(-model[1][2], model[1][1]);
            }
            else
            {
                yaw = std::atan2(model[1][0], model[0][0]);
                pitch = std::atan2(-model[2][0], std::sqrt(model[2][1] * model[2][1] + model[2][2] * model[2][2]));
                roll = std::atan2(model[2][1], model[2][2]);
            }

            constexpr float rad2deg = 180.0f / std::numbers::pi_v<float>;

            return Vector<float, 3>{ pitch * rad2deg, yaw * rad2deg, roll * rad2deg };
        }

        void AddOnPositionChangedCallback(const std::function<void(Vector<float, 3>)>& function)
        {
            onPositionUpdated.push_back(function);
        }

        void AddOnRotationChangedCallback(const std::function<void(Vector<float, 3>)>& function)
        {
            onRotationUpdated.push_back(function);
        }

        void AddOnScaleChangedCallback(const std::function<void(Vector<float, 3>)>& function)
        {
            onScaleUpdated.push_back(function);
        }

        [[nodiscard]]
        std::optional<std::weak_ptr<Transform>> GetParent() const
        {
            return parent;
        }

        void SetParent(std::shared_ptr<Transform> newParent)
        {
            if (!newParent)
                parent = std::nullopt;
            else
                parent = std::make_optional<std::weak_ptr<Transform>>(newParent);
        }

        [[nodiscard]]
        Matrix<float, 4, 4> GetModelMatrix() const
        {
            const auto translation = Matrix<float, 4, 4>::Translation(localPosition);
            const auto rotationX = Matrix<float, 4, 4>::RotationX(localRotation.x() * (std::numbers::pi_v<float> / 180.0f));
            const auto rotationY = Matrix<float, 4, 4>::RotationY(localRotation.y() * (std::numbers::pi_v<float> / 180.0f));
            const auto rotationZ = Matrix<float, 4, 4>::RotationZ(localRotation.z() * (std::numbers::pi_v<float> / 180.0f));
            const auto scale = Matrix<float, 4, 4>::Scale(localScale);

            const Matrix<float, 4, 4> localMatrix = translation * rotationZ * rotationY * rotationX * scale;

            if (parent.has_value())
            {
	            if (const auto parentPtr = parent.value().lock())
                    return parentPtr->GetModelMatrix() * localMatrix;
            }

            return localMatrix;
        }

        void Serialize(cereal::BinaryOutputArchive& archive) const override
        {
            archive(localPosition, localRotation, localScale);
        }

        void Deserialize(cereal::BinaryInputArchive& archive) override
        {
            archive(localPosition, localRotation, localScale);

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
            return typeid(Transform).name();
        }

        static std::shared_ptr<Transform> Create(const Vector<float, 3>& position, const Vector<float, 3>& rotation, const Vector<float, 3>& scale)
        {
            std::shared_ptr<Transform> result(new Transform());

            result->localPosition = position;
            result->localRotation = rotation;
            result->localScale = scale;

            return result;
        }

    private:

        Transform() = default;

        friend class MultiVoxel::Independent::ECS::ComponentFactory;

        bool dirty = true;

        std::optional<std::weak_ptr<Transform>> parent;

        std::vector<std::function<void(Vector<float, 3>)>> onPositionUpdated;
        std::vector<std::function<void(Vector<float, 3>)>> onRotationUpdated;
        std::vector<std::function<void(Vector<float, 3>)>> onScaleUpdated;

        Vector<float, 3> localPosition = { 0.0f, 0.0f, 0.0f };
        Vector<float, 3> localRotation = { 0.0f, 0.0f, 0.0f };
        Vector<float, 3> localScale = { 1.0f, 1.0f, 1.0f };
    };

    REGISTER_COMPONENT(Transform);
}