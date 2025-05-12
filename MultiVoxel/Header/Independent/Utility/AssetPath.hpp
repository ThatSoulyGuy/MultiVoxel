#pragma once

#include <string>
#include <format>
#include <utility>
#include <initializer_list>
#include "Independent/Utility/IndexedString.hpp"

namespace MultiVoxel::Independent::Utility
{
	class AssetPath
	{

	public:

		AssetPath(std::string domain, IndexedString localPath) : domain(std::move(domain)), localPath(std::move(localPath)) { }

		AssetPath() = default;

		[[nodiscard]]
		std::string GetDomain() const
		{
			return domain;
		}

		[[nodiscard]]
		IndexedString GetLocalPath() const
		{
			return localPath;
		}

		[[nodiscard]]
		std::string GetFullPath() const
		{
			return std::format("Assets/{}/{}", domain, localPath.operator std::string());
		}

		template <typename Archive>
		void serialize(Archive& archive)
		{
			archive(domain, localPath);
		}

	private:

		std::string domain;
		IndexedString localPath;

	};
}