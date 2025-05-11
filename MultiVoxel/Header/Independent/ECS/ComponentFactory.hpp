#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "Independent/ECS/Component.hpp"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b)      CONCAT_IMPL(a, b)

#define REGISTER_COMPONENT(Type)                                              \
namespace {                                                                    \
    static const bool CONCAT(_component_registered_, __COUNTER__) = [](){     \
        ComponentFactory::Register<Type>();                                    \
        return true;                                                           \
    }();                                                                       \
}

namespace MultiVoxel::Independent::ECS
{
    class ComponentFactory
    {

    public:

        using Creator = std::function<std::shared_ptr<Component>()>;

        template<typename T>
        static void Register()
        {
            const std::string key = typeid(T).name();

            std::lock_guard<std::mutex> lg(GetMutex());

            GetRegistry()[key] = []()
            {
                return std::static_pointer_cast<Component>(std::shared_ptr<T>(new T()));
            };
        }

        static std::shared_ptr<Component> Create(const std::string& typeName)
        {
            std::lock_guard<std::mutex> lg(GetMutex());

            auto& reg = GetRegistry();
            auto it = reg.find(typeName);

            if (it == reg.end())
                return nullptr;

            return it->second();
        }

    private:

        static std::unordered_map<std::string, Creator>& GetRegistry()
        {
            static std::unordered_map<std::string, Creator> registry;

            return registry;
        }

        static std::mutex& GetMutex()
        {
            static std::mutex mutex;

            return mutex;
        }
    };
}