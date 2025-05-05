#pragma once

#include <iostream>
#include <typeindex>
#include <unordered_map>
#include <concepts>
#include <optional>
#include "Independent/Math/Transform.hpp"
#include "Independent/Utility/IndexedString.hpp"
#include "Server/ECS/Component.hpp"

using namespace MultiVoxel::Independent::Math;
using namespace MultiVoxel::Independent::Utility;

namespace MultiVoxel::Server::ECS
{
	template <typename T>
	concept ComponentType = std::derived_from<T, Component> && requires(T a)
	{
		{ a.Initialize() } -> std::same_as<void>;
		{ a.Update() } -> std::same_as<void>;
		{ a.Render() } -> std::same_as<void>;
		{ a.GetGameObject() } -> std::convertible_to<std::shared_ptr<GameObject>>;
	};

	class GameObject final : public std::enable_shared_from_this<GameObject>
	{

	public:

		GameObject(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject& operator=(GameObject&) = delete;

		std::shared_ptr<GameObject> AddChild(std::shared_ptr<GameObject> child)
		{
			child->SetParent(shared_from_this());

			auto name = child->name;

			childMap.insert({ child->name, std::move(child) });

			return childMap[name];
		}

		std::optional<std::shared_ptr<GameObject>> GetChild(const IndexedString& name)
		{
			if (childMap.contains(name))
			{
				std::cerr << "Child map for game object '" << this->name << "' doesn't have game object '" << name << "'!";
				return std::nullopt;
			}

			return std::make_optional<std::shared_ptr<GameObject>>(childMap[name]);
		}

		void RemoveChild(const IndexedString& name)
		{
			if (childMap.contains(name))
			{
				std::cerr << "Child map for game object '" << this->name << "' doesn't have game object '" << name << "'!";
				return;
			}

			childMap.erase(name);
		}

		template <ComponentType T>
		std::shared_ptr<T> AddComponent(std::shared_ptr<T> component)
		{
			if (componentMap.contains(typeid(T)))
			{
				std::cerr << "Component map for game object '" << name << "' already has component '" << typeid(T).name() << "'!";
				return nullptr;
			}

			component->gameObject = shared_from_this();
			component->Initialize();

			componentMap.insert({ typeid(T), std::move(std::static_pointer_cast<Component>(component)) });

			return std::static_pointer_cast<T>(componentMap[typeid(T)]);
		}

		template <ComponentType T>
		std::optional<std::shared_ptr<T>> GetComponent()
		{
			if (!componentMap.contains(typeid(T)))
			{
				std::cerr << "Component map for game object '" << name << "' doesn't contain component '" << typeid(T).name() << "'!";
				return std::nullopt;
			}

			return std::make_optional<std::shared_ptr<T>>(std::static_pointer_cast<T>(componentMap[typeid(T)]));
		}

		std::shared_ptr<Transform> GetTransform()
		{
			return std::static_pointer_cast<Transform>(componentMap[typeid(Transform)]);
		}

		template <ComponentType T>
		bool HasComponent()
		{
			return componentMap.contains(typeid(T));
		}

		template <ComponentType T>
		void RemoveComponent()
		{
			if (!componentMap.contains(typeid(T)))
			{
				std::cerr << "Component map for game object '" << name << "' doesn't contain component '" << typeid(T).name() << "'!";
				return nullptr;
			}

			componentMap.erase(typeid(T));
		}

		void SetParent(std::shared_ptr<GameObject> parent)
		{
			if (parent)
			{
				this->parent = std::make_optional<std::shared_ptr<GameObject>>(parent);

				GetTransform()->SetParent(parent->GetTransform());
			}
			else
			{
				this->parent = std::nullopt;

				GetTransform()->SetParent(nullptr);
			}
		}

		std::optional<std::shared_ptr<GameObject>> GetParent()
		{
			return parent.has_value() ? std::make_optional<std::shared_ptr<GameObject>>(parent.value().lock()) : std::nullopt;
		}

		IndexedString GetName() const
		{
			return name;
		}

		void Update()
		{
			for (auto& [type, component] : componentMap)
				component->Update();

			for (auto& [name, child] : childMap)
				child->Update();
		}

		void Render()
		{
			for (auto& [type, component] : componentMap)
				component->Render();

			for (auto& [name, child] : childMap)
				child->Render();
		}

		static std::shared_ptr<GameObject> Create(const IndexedString& name)
		{
			std::shared_ptr<GameObject> result(new GameObject());

			result->name = name;
			result->AddComponent(Transform::Create({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }));

			return result;
		}

	private:

		GameObject() = default;

		std::optional<std::weak_ptr<GameObject>> parent;

		std::unordered_map<std::type_index, std::shared_ptr<Component>> componentMap;
		std::unordered_map<IndexedString, std::shared_ptr<GameObject>> childMap;

		IndexedString name;

	};
}