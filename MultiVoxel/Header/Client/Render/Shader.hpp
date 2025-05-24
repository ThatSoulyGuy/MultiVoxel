#pragma once

#include <memory>
#include <glad/glad.h>
#include "Independent/ECS/GameObject.hpp"
#include "Independent/ECS/ComponentFactory.hpp"
#include "Independent/Utility/AssetPath.hpp"
#include "Independent/Utility/FileHelper.hpp"
#include "Independent/Utility/IndexedString.hpp"

using namespace MultiVoxel::Independent::ECS;
using namespace MultiVoxel::Independent::Utility;

namespace MultiVoxel::Client::Render
{
	class Shader final : public Component, public INetworkSerializable
	{

	public:

		Shader(const Shader&) = delete;
		Shader(Shader&&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader& operator=(Shader&&) = delete;

		void Bind() const
		{
			glUseProgram(id);
		}

		void SetUniform(const std::string& name, const int value) const
		{
			const GLint location = glGetUniformLocation(id, name.c_str());

			glUniform1i(location, value);
		}

		void SetUniform(const std::string& name, const bool value) const
		{
			SetUniform(name, static_cast<int>(value));
		}

		void SetUniform(const std::string& name, const float value) const
		{
			const GLint location = glGetUniformLocation(id, name.c_str());

			glUniform1f(location, value);
		}

		void SetUniform(const std::string& name, const Vector<float, 2>& value) const
		{
			const GLint location = glGetUniformLocation(id, name.c_str());

			glUniform2f(location, value[0], value[1]);
		}

		void SetUniform(const std::string& name, const Vector<float, 3>& value) const
		{
			const GLint location = glGetUniformLocation(id, name.c_str());

			glUniform3f(location, value[0], value[1], value[2]);
		}

		void SetUniform(const std::string& name, const Matrix<float, 4, 4>& value) const
		{
			const GLint location = glGetUniformLocation(id, name.c_str());

			glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
		}

		void Serialize(cereal::BinaryOutputArchive& archive) const override
		{
			archive(name, localPath, vertexPath, fragmentPath, id);
		}

		void Deserialize(cereal::BinaryInputArchive& archive) override
		{
			archive(name, localPath, vertexPath, fragmentPath, id);

			if (!GetGameObject()->IsAuthoritative())
			{
				vertexData = FileHelper::ReadFile(vertexPath);
				fragmentData = FileHelper::ReadFile(fragmentPath);
			}

			dirty = false;
		}

		void MarkDirty() override
		{
			dirty = true;
		}

		[[nodiscard]]
		bool IsDirty() const override
		{
			return dirty;
		}

		void ClearDirty() override
		{
			dirty = false;
		}

		[[nodiscard]]
		std::string GetComponentTypeName() const override
		{
			return typeid(Shader).name();
		}

		[[nodiscard]]
		IndexedString GetName() const
		{
			return name;
		}

		static std::shared_ptr<Shader> Create(const IndexedString& name, const AssetPath& localPath)
		{
			std::shared_ptr<Shader> result(new Shader());

			result->name = name;
			result->localPath = localPath;
			result->vertexPath = { { localPath.GetDomain() }, std::format("{}Vertex.glsl", localPath.GetLocalPath().operator std::string()) };
			result->fragmentPath = { { localPath.GetDomain() }, std::format("{}Fragment.glsl", localPath.GetLocalPath().operator std::string()) };
			result->vertexData = FileHelper::ReadFile(result->vertexPath);
			result->fragmentData = FileHelper::ReadFile(result->fragmentPath);
			result->dirty = true;

			result->Generate();

			return result;
		}

	private:
		
		Shader() = default;

		friend class MultiVoxel::Independent::ECS::ComponentFactory;

		void Generate()
		{
			if (isGenerated)
				return;

			const GLuint vertex = glCreateShader(GL_VERTEX_SHADER);

			{
				const char* source = vertexData.c_str();

				glShaderSource(vertex, 1, &source, nullptr);
				glCompileShader(vertex);

				GLint status;

				glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);

				if (status != GL_TRUE)
				{
					char buffer[512];

					glGetShaderInfoLog(vertex, 512, nullptr, buffer);

					std::cerr << "[Shader] Vertex compile error: " << buffer << "\n";
				}
			}

			const GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

			{
				const char* source = fragmentData.c_str();

				glShaderSource(fragment, 1, &source, nullptr);
				glCompileShader(fragment);

				GLint status;

				glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);

				if (status != GL_TRUE)
				{
					char buffer[512];

					glGetShaderInfoLog(fragment, 512, nullptr, buffer);

					std::cerr << "[Shader] Fragment compile error: " << buffer << "\n";
				}
			}

			const GLuint program = glCreateProgram();

			glAttachShader(program, vertex);
			glAttachShader(program, fragment);
			glLinkProgram(program);

			{
				GLint status;

				glGetProgramiv(program, GL_LINK_STATUS, &status);

				if (status != GL_TRUE)
				{
					char buffer[512];

					glGetProgramInfoLog(program, 512, nullptr, buffer);

					std::cerr << "[Shader] Link error: " << buffer << "\n";
				}
			}

			glDeleteShader(vertex);
			glDeleteShader(fragment);

			if (id != 0)
				glDeleteProgram(id);

			id = program;

			isGenerated = true;
		}

		bool dirty{};
		bool isGenerated{};

		GLuint id { 0 };

		IndexedString name;

		AssetPath localPath;

		AssetPath vertexPath, fragmentPath;
		std::string vertexData, fragmentData;

	};

	REGISTER_COMPONENT(Shader)
}