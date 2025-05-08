#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <cstring>

namespace MultiVoxel::Independent::Network
{
    class Message
    {

    public:

        enum class Type : uint8_t
        {
            ConnectRequest = 0,
            ConnectAccept = 1,
            Snapshot = 2,
            InputCommand = 3,
            ChatText = 4,
            Custom = 255
        };

        Message() = default;

        Type GetType() const
        {
            return type;
        }

        const std::vector<uint8_t>& GetBuffer() const
        {
            return buffer;
        }

        bool IsReliable() const
        {
            return reliable;
        }

        void Serialize(std::vector<uint8_t>& out) const
        {
            out.resize(buffer.size() + 1);
            out[0] = static_cast<uint8_t>(type);

            if (!buffer.empty())
                std::memcpy(out.data() + 1, buffer.data(), buffer.size());
        }

        static Message Create(Type type, const void* payload, size_t size, bool reliable = true)
        {
            Message result = { };

            std::vector<uint8_t> buf(size);

            if (payload && size)
                std::memcpy(buf.data(), payload, size);

            result.type = type;
            result.buffer = std::move(buf);
            result.reliable = reliable;

            return result;
        }

    private:

        Type type = Type::Custom;

        std::vector<uint8_t> buffer;

        bool reliable = false;
    };
}