#pragma once

#include <string>
#include <format>
#include <utility>
#include <filesystem>
#include "Independent/Utility/IndexedString.hpp"

#ifdef __APPLE__
  #include <mach-o/dyld.h>
#endif

inline std::string GetExecutableDirectory()
{
#ifdef _WIN32
	return "./";
#elif defined(__APPLE__)
	char buffer[PATH_MAX];

	uint32_t size = sizeof(buffer);

	if (_NSGetExecutablePath(buffer, &size) == 0)
	{
		const std::string fullPath(buffer);

		return fullPath.substr(0, fullPath.find_last_of('/'));
	}

	throw std::runtime_error("Failed to get exe path");
#else
	return "";
#endif
}

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
			return std::format("{}/Assets/{}/{}", GetExecutableDirectory(), domain.operator std::string(), localPath);
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