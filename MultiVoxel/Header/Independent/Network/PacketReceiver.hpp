#pragma once

#include <string>
#include "Independent/Utility/IndexedString.hpp"

using namespace MultiVoxel::Independent::Utility;

namespace MultiVoxel::Independent::Network
{
    class PacketReceiver
    {

    public:

        virtual ~PacketReceiver() = default;

        virtual void OnPacketReceived(const std::string&, const std::string&) = 0;
    };
}