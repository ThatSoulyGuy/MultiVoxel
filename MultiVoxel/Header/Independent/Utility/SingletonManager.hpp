#pragma once

#include <concepts>

namespace MultiVoxel::Independent::Utility
{
	template <typename T, typename A, typename B>
	concept SingletonManager = requires(T x, A y, B b)
	{
		{ x.Register(y) } -> std::convertible_to<A>;
		{ x.Unregister(b) } -> std::same_as<void>;
		{ x.Has(b) } -> std::same_as<bool>;
		{ x.Get(b) } -> std::convertible_to<std::optional<A>>;
		{ x.GetAll() } -> std::same_as<std::vector<A>>;
	};
}