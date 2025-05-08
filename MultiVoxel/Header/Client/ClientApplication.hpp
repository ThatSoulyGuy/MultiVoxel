#pragma once

#include <mutex>
#include <memory>
#include "Client/Core/Window.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"

using namespace MultiVoxel::Client::Core;
using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Client
{
	class ClientApplication final
	{

	public:

		ClientApplication(const ClientApplication&) = delete;
		ClientApplication(ClientApplication&&) = delete;
		ClientApplication& operator=(const ClientApplication&) = delete;
		ClientApplication& operator=(ClientApplication&&) = delete;

		void Preinitialize()
		{
			Window::GetInstance().Initialize("MultiVoxel* 2.1.1", { 750, 450 });

			ComponentFactory::Register<Transform>();
			ComponentFactory::Register<Server::TestComponent>();
		}

		void Initialize()
		{

		}

		bool IsRunning()
		{
			return Window::GetInstance().IsRunning();
		}

		void Update()
		{
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
				instance = std::unique_ptr<ClientApplication>(new ClientApplication());
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