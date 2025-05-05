#pragma once

#include <functional>
#include <unordered_map>
#include "Independent/Network/Message.hpp"

namespace MultiVoxel::Independent::Network
{
    class PeerConnection;

    class MessageDispatcher
    {

    public:

        using HandlerFn = std::function<void(PeerConnection&, const Message&)>;

        void RegisterHandler(Message::Type type, HandlerFn handler)
        {
            handlerMap[type] = std::move(handler);
        }

        void Dispatch(PeerConnection& peer, const Message& msg)
        {
            auto it = handlerMap.find(msg.GetType());

            if (it != handlerMap.end())
                it->second(peer, msg);
        }

    private:

        std::unordered_map<Message::Type, HandlerFn> handlerMap;

    };
}