#pragma once

#include <mutex>
#include <memory>
#include "Client/Core/Window.hpp"
#include "Client/Packet/RpcClient.hpp"
#include "Client/ClientBase.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Thread/MainThreadExecutor.hpp"
#include "Server/ServerApplication.hpp"

using namespace MultiVoxel::Client::Core;
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

	class ClientApplication final
	{

	public:

		ClientApplication(const ClientApplication&) = delete;
		ClientApplication(ClientApplication&&) = delete;
		ClientApplication& operator=(const ClientApplication&) = delete;
		ClientApplication& operator=(ClientApplication&&) = delete;

		void Preinitialize()
		{
			ClientBase::GetInstance().RegisterPacketReceiver(Settings::GetInstance().REPLICATION_RECEIVER.Get());
			ClientBase::GetInstance().RegisterPacketSender((PacketSender*) []()
			{
				auto& temp = RpcClient::GetInstance();

				return &temp;
			}());

			ClientBase::GetInstance().RegisterPacketReceiver((PacketReceiver*)[]()
			{
				auto& temp = RpcClient::GetInstance();

				return &temp;
			}());

			Window::GetInstance().Initialize("MultiVoxel* 2.4.2", { 750, 450 });

			ComponentFactory::Register<Transform>();
			ComponentFactory::Register<Server::TestComponent>();
			ComponentFactory::Register<SomeOtherComponent>();
		}

		void Initialize()
		{
			auto future = RpcClient::GetInstance().CreateGameObjectAsync("default.hi", 0);

			std::thread([future = std::move(future)]() mutable
			{
				auto gameObject = future.get();
					
				MainThreadInvoker::EnqueueTask([&]()
				{
					gameObject->AddComponent(std::shared_ptr<SomeOtherComponent>(new SomeOtherComponent()));
				});
			}).detach();
		}

		void Update()
		{
			MainThreadInvoker::ExecuteTasks();

			GameObjectManager::GetInstance().Update();
		}

		void Render()
		{
			Window::GetInstance().Clear();

			GameObjectManager::GetInstance().Render();

			Window::GetInstance().Present();
		}

		void Uninitialize()
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

					ClientInterfaceLayer::GetInstance().HookEvent("preinitialize", [&]() { result->Preinitialize(); });
					ClientInterfaceLayer::GetInstance().HookEvent("initialize", [&]() { result->Initialize(); });
					ClientInterfaceLayer::GetInstance().HookEvent("update", [&]() { result->Update(); });
					ClientInterfaceLayer::GetInstance().HookEvent("render", [&]() { result->Render(); });
					ClientInterfaceLayer::GetInstance().HookEvent("uninitialize", [&]() { result->Uninitialize(); });

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