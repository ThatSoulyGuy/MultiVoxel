#pragma once

#include "Client/Render/Mesh.hpp"
#include "Client/Render/Vertex.hpp"
#include "Independent/Math/Vector.hpp"

using namespace MultiVoxel::Independent::Math;

namespace MultiVoxel::Client::Render::Vertices
{
    struct FatVertex final
    {
        Vector<float, 3> position;
        Vector<float, 3> color;
        Vector<float, 3> normal;
        Vector<float, 2> uvs;

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(position, color, normal, uvs);
        }

        static VertexBufferLayout GetLayout()
        {
            VertexBufferLayout layout;

            layout.Add(0, 3, GL_FLOAT, GL_FALSE);

            layout.Add(1, 3, GL_FLOAT, GL_FALSE);

            layout.Add(2, 3, GL_FLOAT, GL_FALSE);

            layout.Add(3, 2, GL_FLOAT, GL_FALSE);

            return layout;
        }
    };
}

namespace MultiVoxel::Client::Render
{
    REGISTER_COMPONENT(Mesh<Vertices::FatVertex>);
}