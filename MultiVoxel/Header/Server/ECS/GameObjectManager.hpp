#pragma once

#include "Independent/Utility/SingletonManager.hpp"
#include "Server/ECS/GameObject.hpp"

namespace MultiVoxel::Server::ECS
{
	class GameObjectManager
	{

	public:

		GameObjectManager(const GameObjectManager&) = delete;
		GameObjectManager(GameObjectManager&&) = delete;
		GameObjectManager& operator=(const GameObjectManager&) = delete;
		GameObjectManager& operator=(GameObjectManager&&) = delete;

		std::shared_ptr<GameObject> Register(std::shared_ptr<GameObject> object)
		{
			auto name = object->GetName();

			if (gameObjectMap.contains(name))
			{
				std::cerr << "Game object map already has game object '" << name << "'!";
				return nullptr;
			}

			gameObjectMap.insert({ name, std::move(object) });

			return gameObjectMap[name];
		}

		void Unregister(IndexedString name)
		{
			if (!gameObjectMap.contains(name))
			{
				std::cerr << "Game object map doesn't have game object '" << name << "'!";
				return;
			}

			gameObjectMap.erase(name);
		}

		bool Has(IndexedString name)
		{
			return gameObjectMap.contains(name);
		}

		std::optional<std::shared_ptr<GameObject>> Get(IndexedString name)
		{
			if (!gameObjectMap.contains(name))
			{
				std::cerr << "Game object map doesn't have game object '" << name << "'!";
				return std::nullopt;
			}

			return gameObjectMap.contains(name) ? std::make_optional<std::shared_ptr<GameObject>>(gameObjectMap[name]) : std::nullopt;
		}

		std::vector<std::shared_ptr<GameObject>> GetAll()
		{
			std::vector<std::shared_ptr<GameObject>> result(gameObjectMap.size());

			std::transform(gameObjectMap.begin(), gameObjectMap.end(), result.begin(), [](const auto& pair) { return pair.second; });

			return result;
		}

		void Update()
		{
			for (auto& [name, gameObject] : gameObjectMap)
				gameObject->Update();
		}

		void Render()
		{
			for (auto& [name, gameObject] : gameObjectMap)
				gameObject->Render();
		}

		static GameObjectManager& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<GameObjectManager>(new GameObjectManager());
			});

			return *instance;
		}

	private:

		GameObjectManager() = default;

		std::unordered_map<IndexedString, std::shared_ptr<GameObject>> gameObjectMap;

		static std::once_flag initializationFlag;
		static std::unique_ptr<GameObjectManager> instance;

	};

	std::once_flag GameObjectManager::initializationFlag;
	std::unique_ptr<GameObjectManager> GameObjectManager::instance;

	static_assert(
		SingletonManager<GameObjectManager, std::shared_ptr<GameObject>, IndexedString>,
		"GameObjectManager must implement Register/Unregister/Has/Get/GetAll exactly as the SingletonManager concept requires"
		);
}