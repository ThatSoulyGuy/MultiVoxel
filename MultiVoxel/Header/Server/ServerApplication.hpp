#pragma once

#include <memory>
#include <mutex>
#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"
#include "Server/ServerInterfaceLayer.hpp"

using namespace MultiVoxel::Independent::Core;
using namespace MultiVoxel::Independent::ECS;

namespace MultiVoxel::Server
{
	class ServerApplication final
	{

	public:

		ServerApplication(const ServerApplication&) = delete;
		ServerApplication(ServerApplication&&) = delete;
		ServerApplication& operator=(const ServerApplication&) = delete;
		ServerApplication& operator=(ServerApplication&&) = delete;

		static void Preinitialize()
		{
			ServerBase::GetInstance().RegisterPacketSender(Settings::GetInstance().REPLICATION_SENDER.Get());
		}

		static void Initialize()
		{

		}

		static void Update()
		{
			GameObjectManager::GetInstance().Update();
		}

		static void Uninitialize()
		{
			
		}

		static ServerApplication& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = [&]()
				{
					auto result = std::unique_ptr<ServerApplication>(new ServerApplication());
					
					ServerInterfaceLayer::GetInstance().HookEvent("preinitialize", [&]() { Preinitialize(); });
					ServerInterfaceLayer::GetInstance().HookEvent("initialize", [&]() { Initialize(); });
					ServerInterfaceLayer::GetInstance().HookEvent("update", [&]() { Update(); });
					ServerInterfaceLayer::GetInstance().HookEvent("uninitialize", [&]() { Uninitialize(); });

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