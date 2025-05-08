#pragma once

#include <string>
#include <format>
#include "Independent/Utility/IndexedString.hpp"

namespace MultiVoxel::Independent::Utility
{
	class AssetPath
	{

	public:
		
		AssetPath(const std::string& domain, const IndexedString& localPath) : domain(domain), localPath(localPath) { }

		std::string GetDomain() const
		{
			return domain;
		}

		IndexedString GetLocalPath() const
		{
			return localPath;
		}

		std::string GetFullPath() const
		{
			return std::format("Assets/{}/{}", domain, localPath.operator std::string());
		}

	private:

		std::string domain;
		IndexedString localPath;

	};
}