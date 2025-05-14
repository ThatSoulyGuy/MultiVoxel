#pragma once

#include <string>
#include <format>
#include <utility>
#include <filesystem>
#include "Independent/Utility/IndexedString.hpp"

namespace MultiVoxel::Independent::Utility
{
	class AssetPath
	{

	public:

		AssetPath(IndexedString domain, std::string localPath) : domain(std::move(domain)), localPath(std::move(localPath)) { }

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
			return std::format("./Assets/{}/{}", domain.operator std::string(), localPath);
		}

		template <typename Archive>
		void serialize(Archive& archive)
		{
			archive(domain, localPath);
		}

	private:

		IndexedString domain;
		std::string localPath;

	};
}