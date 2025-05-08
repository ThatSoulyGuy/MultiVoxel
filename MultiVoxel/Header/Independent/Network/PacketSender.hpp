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

        virtual bool SendPacket(std::string&, std::string&) = 0;

        virtual void Reload() = 0;

        bool sent = false;
    };
}