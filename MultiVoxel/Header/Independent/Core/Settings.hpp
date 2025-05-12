#pragma once

#include <mutex>
#include <memory>
#include "Client/Packet/ReplicationReceiver.hpp"
#include "Server/Packet/ReplicationSender.hpp"

using namespace MultiVoxel::Client::Packet;
using namespace MultiVoxel::Server::Packet;

namespace MultiVoxel::Independent::ECS
{
	class Component;
}

namespace MultiVoxel::Independent::Core
{
	template <typename T, bool IS_MUTABLE>
	class Setting final
	{

	public:

		Setting(T value)
		{
			this->value = value;
		}

		void Set(T value) requires (IS_MUTABLE)
		{
			this->value = value;
		}

		T& Get()
		{
			return value;
		}

	private:

		T value;

	};

	class Settings final
	{

	public:

		~Settings()
		{
			delete REPLICATION_RECEIVER.Get();
			delete REPLICATION_SENDER.Get();
		}

		Settings(const Settings&) = delete;
		Settings(Settings&&) = delete;
		Settings& operator=(const Settings&) = delete;
		Settings& operator=(Settings&&) = delete;

		static Settings& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<Settings>(new Settings());
			});

			return *instance;
		}

		Setting<ReplicationReceiver*, false> REPLICATION_RECEIVER = { new ReplicationReceiver() };
		Setting<ReplicationSender*, false> REPLICATION_SENDER = { new ReplicationSender() };

	private:

		Settings() = default;

		static std::once_flag initializationFlag;
		static std::unique_ptr<Settings> instance;

	};

	std::once_flag Settings::initializationFlag;
	std::unique_ptr<Settings> Settings::instance;
}