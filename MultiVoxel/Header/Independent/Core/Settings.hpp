#pragma once

#include <iostream>
#include <mutex>
#include <memory>
#include <unordered_map>
#include "Client/ReplicationReceiver.hpp"
#include "Server/ReplicationSender.hpp"

using namespace MultiVoxel::Client;
using namespace MultiVoxel::Server;

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

		static Settings& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<Settings>(new Settings());
			});

			return *instance;
		}

		Setting<ReplicationReceiver, false> REPLICATION_RECEIVER = { ReplicationReceiver() };
		Setting<ReplicationSender, false> REPLICATION_SENDER = { ReplicationSender() };

	private:

		Settings() = default;

		static std::once_flag initializationFlag;
		static std::unique_ptr<Settings> instance;

	};

	std::once_flag Settings::initializationFlag;
	std::unique_ptr<Settings> Settings::instance;
}