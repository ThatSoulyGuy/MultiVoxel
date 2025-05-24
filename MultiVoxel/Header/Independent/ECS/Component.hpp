#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <cereal/cereal.hpp>

namespace MultiVoxel::Independent::Math
{
	class Camera;
}

namespace MultiVoxel::Independent::ECS
{
	using NetworkId = uint32_t;

	inline std::atomic<NetworkId> NextNetworkIdCounter{ 1 };

	inline NetworkId GetNextNetworkId()
	{
		return NextNetworkIdCounter.fetch_add(1, std::memory_order_relaxed);
	}

	inline const std::string SyncChannelName = "NetSyncChannel";

	struct INetworkSerializable
	{
		virtual ~INetworkSerializable() = default;

		virtual void Serialize(cereal::BinaryOutputArchive& ar) const = 0;
		virtual void Deserialize(cereal::BinaryInputArchive& ar) = 0;

		virtual void MarkDirty() = 0;

		[[nodiscard]]
		virtual bool IsDirty() const = 0;

		virtual void ClearDirty() = 0;

		[[nodiscard]]
		virtual std::string GetComponentTypeName() const = 0;
	};

	class GameObject;

	class Component
	{

	public:

		virtual ~Component() = default;

		virtual void Initialize() { }
		
		virtual void Update() { }

		virtual void Render(const std::shared_ptr<MultiVoxel::Independent::Math::Camera>&) { }

		[[nodiscard]]
		std::shared_ptr<GameObject> GetGameObject() const
		{
			return gameObject.lock();
		}

	private:

		std::weak_ptr<GameObject> gameObject;

		friend class GameObject;

	};
}
