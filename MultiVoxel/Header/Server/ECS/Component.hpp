#pragma once

#include <memory>

namespace MultiVoxel::Server::ECS
{
	class GameObject;

	class Component
	{

	public:

		virtual void Initialize() { }
		
		virtual void Update() { }

		virtual void Render() { }

		std::shared_ptr<GameObject> GetGameObject() const
		{
			return gameObject.lock();
		}

	private:

		std::weak_ptr<GameObject> gameObject;

		friend class GameObject;

	};
}