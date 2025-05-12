#pragma once

#include "Independent/Utility/SingletonManager.hpp"
#include "Independent/ECS/GameObject.hpp"

namespace MultiVoxel::Independent::ECS
{
	class GameObjectManager final : SingletonManager<std::shared_ptr<GameObject>, const IndexedString&>
	{

	public:

		GameObjectManager(const GameObjectManager&) = delete;
		GameObjectManager(GameObjectManager&&) = delete;
		GameObjectManager& operator=(const GameObjectManager&) = delete;
		GameObjectManager& operator=(GameObjectManager&&) = delete;

		std::shared_ptr<GameObject> Register(std::shared_ptr<GameObject> object) override
		{
			auto name = object->GetName();

			if (gameObjectMap.contains(name))
			{
				std::cerr << "Game object map already has game object '" << name << "'!";
				return nullptr;
			}

			gameObjectMap.insert({ name, std::move(object) });
			networkMap.insert({ gameObjectMap[name]->GetNetworkId(), gameObjectMap[name] });

			return gameObjectMap[name];
		}

		void Unregister(const IndexedString& name) override
		{
			if (!gameObjectMap.contains(name))
			{
				std::cerr << "Game object map doesn't have game object '" << name << "'!";
				return;
			}

			networkMap.erase(gameObjectMap[name]->GetNetworkId());

			gameObjectMap.erase(name);
		}

		void Unregister(const NetworkId id)
		{
			if (!networkMap.contains(id))
			{
				std::cerr << "Game object map doesn't have game object '" << id << "'!";
				return;
			}

			gameObjectMap.erase(networkMap[id].lock()->GetName());
			
			networkMap.erase(id);
		}

		bool Has(const IndexedString& name) const override
		{
			return gameObjectMap.contains(name);
		}

		bool Has(const NetworkId id) const
		{
			return networkMap.contains(id);
		}

		std::optional<std::shared_ptr<GameObject>> Get(const IndexedString& name) override
		{
			if (!gameObjectMap.contains(name))
			{
				std::cerr << "Game object map doesn't have game object '" << name << "'!";
				return std::nullopt;
			}

			return gameObjectMap.contains(name) ? std::make_optional<std::shared_ptr<GameObject>>(gameObjectMap[name]) : std::nullopt;
		}

		std::optional<std::shared_ptr<GameObject>> Get(const NetworkId& id)
		{
			if (!networkMap.contains(id))
			{
				std::cerr << "Game object map doesn't have game object '" << id << "'!";
				return std::nullopt;
			}

			return networkMap.contains(id) ? std::make_optional<std::shared_ptr<GameObject>>(networkMap[id]) : std::nullopt;
		}

		std::vector<std::shared_ptr<GameObject>> GetAll() const override
		{
			std::vector<std::shared_ptr<GameObject>> result(gameObjectMap.size());

			std::ranges::transform(gameObjectMap, result.begin(), [](const auto& pair) { return pair.second; });

			return result;
		}

		void Update()
		{
			for (const auto& gameObject: gameObjectMap | std::views::values)
				gameObject->Update();
		}

		void Render()
		{
			for (const auto& gameObject: gameObjectMap | std::views::values)
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
		std::unordered_map<NetworkId, std::weak_ptr<GameObject>> networkMap;

		static std::once_flag initializationFlag;
		static std::unique_ptr<GameObjectManager> instance;

	};

	std::once_flag GameObjectManager::initializationFlag;
	std::unique_ptr<GameObjectManager> GameObjectManager::instance;
}