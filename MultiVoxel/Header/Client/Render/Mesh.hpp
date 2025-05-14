#pragma once

#include <vector>
#include <memory>
#include <concepts>
#include <glad/glad.h>
#include "Client/Render/Shader.hpp"
#include "Client/Render/Vertex.hpp"
#include "Independent/ECS/Component.hpp"
#include "Independent/ECS/ComponentFactory.hpp"

namespace MultiVoxel::Client::Render
{
    template <typename T>
    concept VertexType = requires(T a)
    {
        { a.GetLayout() } -> std::same_as<VertexBufferLayout>;
    };

    template <VertexType T>
    class Mesh final : public Component, public INetworkSerializable, public std::enable_shared_from_this<Mesh<T>>
    {

    public:

        ~Mesh() override
        {
            if (VAO)
                glDeleteVertexArrays(1, &VAO);

            if (VBO)
                glDeleteBuffers(1, &VBO);

            if (EBO)
                glDeleteBuffers(1, &EBO);
        }

        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        void Generate()
        {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(T), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

            const VertexBufferLayout layout = T::GetLayout();
            const GLuint stride = layout.GetStride();

            for (const auto& [index, componentCount, type, normalized, offset] : layout.GetElements())
            {
                glEnableVertexAttribArray(index);
                glVertexAttribPointer(index, componentCount, type, normalized, static_cast<GLsizei>(stride), reinterpret_cast<const void*>(offset));
            }

            glBindVertexArray(0);
        }

        void Render() override
        {
            if (!GetGameObject()->template GetComponent<Shader>().has_value())
                return;

            GetGameObject()->template GetComponent<Shader>().value()->Bind();

            glBindVertexArray(VAO);

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

            glBindVertexArray(0);
        }

        void SetVertices(const std::vector<T>& vertices)
        {
            MarkDirty();

            this->vertices = vertices;
        }

        void SetIndices(const std::vector<uint32_t>& indices)
        {
            MarkDirty();

            this->indices = indices;
        }

        void Serialize(cereal::BinaryOutputArchive& archive) const override
        {
            archive(vertices, indices);
        }

        void Deserialize(cereal::BinaryInputArchive& archive) override
        {
            archive(vertices, indices);

            if (!GetGameObject()->IsAuthoritative())
                Generate();
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
            return typeid(Mesh).name();
        }

        static std::shared_ptr<Mesh> Create(const std::vector<T>& vertices, const std::vector<uint32_t>& indices)
        {
            auto result = std::shared_ptr<Mesh>(new Mesh());

            result->vertices = vertices;
            result->indices = indices;

            return result;
        }

    private:

        Mesh() = default;

        friend class MultiVoxel::Independent::ECS::ComponentFactory;

        bool dirty = true;

        std::vector<T> vertices;
        std::vector<uint32_t> indices;

        GLuint VAO{0}, VBO{0}, EBO{0};
    };

    REGISTER_COMPONENT_TEMPLATED(Mesh);
}