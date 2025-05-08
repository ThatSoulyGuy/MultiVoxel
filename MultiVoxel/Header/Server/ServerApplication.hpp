#pragma once

#include <mutex>
#include <memory>
#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"
#include "Server/ServerInterfaceLayer.hpp"

using namespace MultiVoxel::Independent::Core;
using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Server
{
	class TestComponent final : public Component, public INetworkSerializable
	{

	public:

		bool dirty = true;

		void Initialize() override
		{
			std::cout << "Hi :D" << std::endl;
		}

		void Update() override
		{
			if (GetGameObject()->IsAuthoritative())
				std::cout << "Hi from server" << std::endl;
			else
				std::cout << "Hi from client" << std::endl;
		}

		void Serialize(cereal::BinaryOutputArchive& ar) const override { }

		void Deserialize(cereal::BinaryInputArchive& ar) override { }

		void MarkDirty()
		{
			dirty = true;
		}

		bool IsDirty() const override
		{
			return dirty;
		}

		void ClearDirty() override
		{
			dirty = false;
		}

		std::string GetComponentTypeName() const override
		{
			return typeid(TestComponent).name();
		}
	};

	class ServerApplication final
	{

	public:

		ServerApplication(const ServerApplication&) = delete;
		ServerApplication(ServerApplication&&) = delete;
		ServerApplication& operator=(const ServerApplication&) = delete;
		ServerApplication& operator=(ServerApplication&&) = delete;

		void Preinitialize()
		{
			ServerBase::GetInstance().RegisterPacketSender(Settings::GetInstance().REPLICATION_SENDER.Get());
		}

		void Initialize()
		{
			auto square = GameObjectManager::GetInstance().Register(GameObject::Create({ "default.some" }));

			square->AddComponent(std::shared_ptr<TestComponent>(new TestComponent));

			Settings::GetInstance().REPLICATION_SENDER.Get()->QueueSpawn(square);
		}

		void Update()
		{
			GameObjectManager::GetInstance().Update();
		}

		void Render()
		{
			GameObjectManager::GetInstance().Render();
		}

		void Uninitialize()
		{
			
		}

		static ServerApplication& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = [&]()
				{
					auto result = std::unique_ptr<ServerApplication>(new ServerApplication());
					
					ServerInterfaceLayer::GetInstance().HookEvent("preinitialize", [&]() { result->Preinitialize(); });
					ServerInterfaceLayer::GetInstance().HookEvent("initialize", [&]() { result->Initialize(); });
					ServerInterfaceLayer::GetInstance().HookEvent("update", [&]() { result->Update(); });
					ServerInterfaceLayer::GetInstance().HookEvent("render", [&]() { result->Render(); });
					ServerInterfaceLayer::GetInstance().HookEvent("uninitialize", [&]() { result->Uninitialize(); });

					return std::move(result);
				}();
			});

			return *instance;
		}

	private:

		ServerApplication() = default;

		static std::once_flag initializationFlag;
		static std::unique_ptr<ServerApplication> instance;

	};

	std::once_flag ServerApplication::initializationFlag;
	std::unique_ptr<ServerApplication> ServerApplication::instance;
}