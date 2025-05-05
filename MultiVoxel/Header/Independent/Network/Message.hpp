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
            return mType;
        }

        const std::vector<uint8_t>& GetBuffer() const
        {
            return mBuffer;
        }

        bool IsReliable() const
        {
            return mReliable;
        }

        void Serialize(std::vector<uint8_t>& out) const
        {
            out.resize(mBuffer.size() + 1);
            out[0] = static_cast<uint8_t>(mType);

            if (!mBuffer.empty())
                std::memcpy(out.data() + 1, mBuffer.data(), mBuffer.size());
        }

        static Message Create(Type type, const void* payload, size_t size, bool reliable = true)
        {
            std::vector<uint8_t> buf(size);

            if (payload && size)
                std::memcpy(buf.data(), payload, size);

            return Message(type, std::move(buf), reliable);
        }

    private:

        Message(Type t, std::vector<uint8_t>&& buf, bool reliable) : mType(t), mBuffer(std::move(buf)), mReliable(reliable) { }

        Type mType;
        std::vector<uint8_t> mBuffer;
        bool mReliable;
    };
}