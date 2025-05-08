#pragma once

#include <mutex>
#include <memory>
#include <unordered_map>
#include <functional>

namespace MultiVoxel::Server
{
	class ServerInterfaceLayer final
	{

	public:

		ServerInterfaceLayer(const ServerInterfaceLayer&) = delete;
		ServerInterfaceLayer(ServerInterfaceLayer&&) = delete;
		ServerInterfaceLayer& operator=(const ServerInterfaceLayer&) = delete;
		ServerInterfaceLayer& operator=(ServerInterfaceLayer&&) = delete;

		void HookEvent(const std::string& name, const std::function<void()>& event)
		{
			eventHookMap.insert({ name, event });
		}

		void CallEvent(const std::string& name)
		{
			eventHookMap[name]();
		}

		static ServerInterfaceLayer& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<ServerInterfaceLayer>(new ServerInterfaceLayer());
			});

			return *instance;
		}

	private:

		ServerInterfaceLayer() = default;
		
		std::unordered_map<std::string, std::function<void()>> eventHookMap;

		static std::once_flag initializationFlag;
		static std::unique_ptr<ServerInterfaceLayer> instance;

	};

	std::once_flag ServerInterfaceLayer::initializationFlag;
	std::unique_ptr<ServerInterfaceLayer> ServerInterfaceLayer::instance;
}