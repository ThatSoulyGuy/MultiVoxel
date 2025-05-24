#pragma once

#include <mutex>
#include <memory>
#include "Client/Core/InputManager.hpp"
#include "Client/Core/Window.hpp"
#include "Client/Render/Vertices/FatVertex.hpp"
#include "Client/Render/Mesh.hpp"
#include "Client/Render/ShaderManager.hpp"
#include "Client/ClientBase.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Thread/MainThreadExecutor.hpp"
#include "Server/Entity/Entities/EntityPlayer.hpp"

#include "Server/ServerApplication.hpp"

using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Client::Render::Vertices;
using namespace MultiVoxel::Client::Render;
using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Thread;
using namespace MultiVoxel::Server::Entity::Entities;

namespace MultiVoxel::Client
{
	class SomeOtherComponent final : public Component
	{

	public:

		void Initialize() override
		{
			std::cout << "hi from some other component :D" << std::endl;
		}
	};

	REGISTER_COMPONENT(SomeOtherComponent);

	class ClientApplication final
	{

	public:

		ClientApplication(const ClientApplication&) = delete;
		ClientApplication(ClientApplication&&) = delete;
		ClientApplication& operator=(const ClientApplication&) = delete;
		ClientApplication& operator=(ClientApplication&&) = delete;

		static void Preinitialize()
		{
			ClientBase::GetInstance().RegisterPacketReceiver(Settings::GetInstance().REPLICATION_RECEIVER.Get());
			ClientBase::GetInstance().RegisterPacketSender([]
			{
				auto& temp = RpcClient::GetInstance();

				return &temp;
			}());

			ClientBase::GetInstance().RegisterPacketReceiver([]
			{
				auto& temp = RpcClient::GetInstance();

				return &temp;
			}());

			Window::GetInstance().Initialize("MultiVoxel* 3.12.2", { 750, 450 });

			InputManager::GetInstance().Initialize();

			ShaderManager::GetInstance().Register(Shader::Create({ "multivoxel.fat" }, { { "MultiVoxel" }, "Shader/Fat" }));
		}

		static void Initialize()
		{
			auto futureFloor = RpcClient::GetInstance().CreateGameObjectAsync("default.floor", 0);

			std::thread([future = std::move(futureFloor)]() mutable
			{
				const auto gameObject = future.get();

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

				std::cout << "Got here" << std::endl;
			}).detach();

			auto futurePlayer = RpcClient::GetInstance().CreateGameObjectAsync("default.player", 0);

			std::thread([future = std::move(futurePlayer)]() mutable
			{
				const auto gameObject = future.get();

				RpcClient::GetInstance().MoveGameObject(gameObject->GetNetworkId(), { 0.0f, 1.0f, -3.0f }, { 0.0f, 0.0f, 0.0f });
				RpcClient::GetInstance().AddComponentAsync(gameObject->GetNetworkId(), EntityPlayer::Create());
				camera = RpcClient::GetInstance().AddComponentAsync(gameObject->GetNetworkId(), Camera::Create(45.0f, 0.01f, 1000.0f)).get();
			}).detach();
		}

		static void Update()
		{
			MainThreadInvoker::ExecuteTasks();

			GameObjectManager::GetInstance().Update();

			InputManager::GetInstance().Update();
		}

		static void Render()
		{
			Window::Clear();

			GameObjectManager::GetInstance().Render(camera);

			Window::GetInstance().Present();
		}

		static void Uninitialize()
		{
			Window::GetInstance().Uninitialize();
		}

		static ClientApplication& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = [&]()
				{
					auto result = std::unique_ptr<ClientApplication>(new ClientApplication());

					ClientInterfaceLayer::GetInstance().HookEvent("preinitialize", [&]() { Preinitialize(); });
					ClientInterfaceLayer::GetInstance().HookEvent("initialize", [&]() { Initialize(); });
					ClientInterfaceLayer::GetInstance().HookEvent("update", [&]() { Update(); });
					ClientInterfaceLayer::GetInstance().HookEvent("render", [&]() { Render(); });
					ClientInterfaceLayer::GetInstance().HookEvent("uninitialize", [&]() { Uninitialize(); });

					return std::move(result);
				}();
			});

			return *instance;
		}

	private:

		ClientApplication() = default;

		static std::shared_ptr<Camera> camera;

		static std::once_flag initializationFlag;
		static std::unique_ptr<ClientApplication> instance;

	};

	std::shared_ptr<Camera> ClientApplication::camera;
	std::once_flag ClientApplication::initializationFlag;
	std::unique_ptr<ClientApplication> ClientApplication::instance;
}