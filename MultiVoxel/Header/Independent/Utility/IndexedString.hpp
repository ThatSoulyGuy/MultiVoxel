#pragma once

#include <functional>
#include <cstddef>
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <cereal/cereal.hpp>

namespace MultiVoxel::Independent::Utility
{
	class IndexedString
	{

	public:

		IndexedString(const std::string& input)
		{
			data = Split(input, '.');
		}

		IndexedString() = default;

		std::string operator[](const size_t index) const
		{
			return data[index];
		}

		[[nodiscard]]
		size_t Length() const
		{
			return data.size();
		}

		operator std::string() const
		{
			std::ostringstream buffer;

			for (size_t i = 0; i < data.size(); ++i)
			{
				buffer << data[i];

				if (i + 1 < data.size())
					buffer << '.';
			}

			return buffer.str();
		}

	private:

		static std::vector<std::string> Split(std::string input, char delimiter)
		{
			std::vector<std::string> tokens;

			size_t position = 0;

			while ((position = input.find(delimiter)) != std::string::npos)
			{
				tokens.emplace_back(input.substr(0, position));
				input.erase(0, position + 1);
			}

			tokens.emplace_back(input);
			return tokens;
		}

		std::vector<std::string> data;

	};

	inline bool operator==(const IndexedString& lhs, const IndexedString& rhs)
	{
		return lhs.Length() == rhs.Length() && [&]
		{
			for (size_t i = 0; i < lhs.Length(); ++i)
			{
				if (lhs[i] != rhs[i])
					return false;
			}

			return true;
		}();
	}

	inline bool operator!=(const IndexedString& lhs, const IndexedString& rhs)
	{
		return !(lhs == rhs);
	}

	inline std::ostream& operator<<(std::ostream& stream, const IndexedString& input)
	{
		stream << static_cast<std::string>(input);

		return stream;
	}
}

template<>
struct std::hash<MultiVoxel::Independent::Utility::IndexedString>
{
	std::size_t operator()(MultiVoxel::Independent::Utility::IndexedString const& key) const noexcept
	{
		std::size_t seed = 0;

		for (size_t i = 0; i < key.Length(); ++i)
		{
			std::hash<std::string> hasher;
			const auto& token = key[i];

			seed ^= hasher(token) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
		}

		return seed;
	}
};

namespace cereal
{
	template <class Archive>
	void save(Archive& ar, const MultiVoxel::Independent::Utility::IndexedString& string)
	{
		ar(string.operator std::string());
	}

	template <class Archive>
	void load(Archive& ar, MultiVoxel::Independent::Utility::IndexedString& is)
	{
		std::string string;

		ar(string);

		is = MultiVoxel::Independent::Utility::IndexedString(string);
	}
}