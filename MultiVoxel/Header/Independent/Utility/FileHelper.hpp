#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include "Independent/Utility/AssetPath.hpp"

namespace MultiVoxel::Independent::Utility
{
	class FileHelper
	{

	public:

		FileHelper(const FileHelper&) = delete;
		FileHelper(FileHelper&&) = delete;
		FileHelper& operator=(const FileHelper&) = delete;
		FileHelper& operator=(FileHelper&&) = delete;

		static std::string ReadFile(const AssetPath& path)
		{
            std::ifstream file(path.GetFullPath(), std::ios::binary | std::ios::ate);

            if (!file.is_open())
            {
                std::cout << "Failed to read file '" << path.GetFullPath() << "'!\n";
                return {};
            }

            auto fileSize = file.tellg();
            file.seekg(0);

            std::string result;

            result.resize(static_cast<std::size_t>(fileSize));

            if (!file.read(result.data(), fileSize))
            {
                std::cout << "Failed to read contents of '" << path.GetFullPath() << "'!\n";
                return {};
            }

            return result;
		}

	private:

		FileHelper() = default;

	};
}