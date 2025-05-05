#pragma once

#include <string>
#include "Independent/Utility/IndexedString.hpp"

using namespace MultiVoxel::Independent::Utility;

namespace MultiVoxel::Independent::Network
{
    class PacketSender
    {

    public:

        virtual ~PacketSender() = default;

        virtual bool SendPacket(IndexedString&, std::string&) = 0;
    };
}