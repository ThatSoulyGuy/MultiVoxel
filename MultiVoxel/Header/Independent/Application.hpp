#pragma once

#include <mutex>
#include <memory>
#include "Client/Core/Window.hpp"

using namespace MultiVoxel::Client::Core;

namespace MultiVoxel::Independent
{
	class Application final
	{

	public:

		Application(const Application&) = delete;
		Application(Application&&) = delete;
		Application& operator=(const Application&) = delete;
		Application& operator=(Application&&) = delete;

		void Preinitialize()
		{
			Window::GetInstance().Initialize("MultiVoxel* 2.1.1", { 750, 450 });
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

		}

		void Render()
		{
			Window::GetInstance().Clear();
			Window::GetInstance().Present();
		}

		void Uninitialize()
		{
			Window::GetInstance().Uninitialize();
		}

		static Application& GetInstance()
		{
			std::call_once(initializationFlag, [&]()
			{
				instance = std::unique_ptr<Application>(new Application());
			});

			return *instance;
		}

	private:

		Application() = default;

		static std::once_flag initializationFlag;
		static std::unique_ptr<Application> instance;

	};

	std::once_flag Application::initializationFlag;
	std::unique_ptr<Application> Application::instance;
}