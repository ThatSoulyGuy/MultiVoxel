#pragma once

#include <mutex>
#include <memory>
#include "Client/Core/Window.hpp"
#include "Client/Render/Vertices/FatVertex.hpp"
#include "Client/Render/Mesh.hpp"
#include "Client/Render/ShaderManager.hpp"
#include "Client/ClientBase.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Thread/MainThreadExecutor.hpp"

#include "Server/ServerApplication.hpp"

using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Client::Render::Vertices;
using namespace MultiVoxel::Client::Render;
using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Thread;

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

			Window::GetInstance().Initialize("MultiVoxel* 2.8.2", { 750, 450 });

			ShaderManager::GetInstance().Register(Shader::Create({ "multivoxel.fat" }, { { "MultiVoxel" }, "Shader/Fat" }));
		}

		static void Initialize()
		{
			auto futureGameObject = RpcClient::GetInstance().CreateGameObjectAsync("default.test", 0);

			std::thread([future = std::move(futureGameObject)]() mutable
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
					0, 2, 3
				}));

				component.get()->Generate();
				std::cout << "Got here" << std::endl;
			}).detach();
		}

		static void Update()
		{
			MainThreadInvoker::ExecuteTasks();

			GameObjectManager::GetInstance().Update();
		}

		static void Render()
		{
			Window::Clear();

			GameObjectManager::GetInstance().Render();

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

		static std::once_flag initializationFlag;
		static std::unique_ptr<ClientApplication> instance;

	};

	std::once_flag ClientApplication::initializationFlag;
	std::unique_ptr<ClientApplication> ClientApplication::instance;
}