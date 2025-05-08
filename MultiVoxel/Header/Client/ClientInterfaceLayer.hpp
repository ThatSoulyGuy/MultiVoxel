#pragma once

#include <mutex>
#include <memory>
#include <unordered_map>
#include <functional>

namespace MultiVoxel::Client
{
	class ClientInterfaceLayer final
	{

	public:

		ClientInterfaceLayer(const ClientInterfaceLayer&) = delete;
		ClientInterfaceLayer(ClientInterfaceLayer&&) = delete;
		ClientInterfaceLayer& operator=(const ClientInterfaceLayer&) = delete;
		ClientInterfaceLayer& operator=(ClientInterfaceLayer&&) = delete;

		void HookEvent(const std::string& name, const std::function<void()>& event)
		{
			eventHookMap.insert({ name, event });
		}

		void CallEvent(const std::string& name)
		{
			eventHookMap[name]();
		}

		static ClientInterfaceLayer& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<ClientInterfaceLayer>(new ClientInterfaceLayer());
			});

			return *instance;
		}

	private:

		ClientInterfaceLayer() = default;
		
		std::unordered_map<std::string, std::function<void()>> eventHookMap;

		static std::once_flag initializationFlag;
		static std::unique_ptr<ClientInterfaceLayer> instance;

	};

	std::once_flag ClientInterfaceLayer::initializationFlag;
	std::unique_ptr<ClientInterfaceLayer> ClientInterfaceLayer::instance;
}