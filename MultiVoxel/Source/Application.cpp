#include "Client/ClientBase.hpp"
#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"

struct PingSender : PacketSender
{
    bool SendPacket(std::string& outName, std::string& outData) override
    {
        if (sent)
            return false;

        outName = IndexedString("PingChannel");
        outData = "Hello from server";

        sent = true;

        return true;
    }
};

struct PingReceiver : PacketReceiver
{
    void OnPacketReceived(const std::string& name,
        const std::string& data) override {
        if (name == IndexedString("PingChannel"))
            std::cout << "[Client] Got: " << data << "\n";
    }
};

int main(void)
{
    std::cout << "Run as (s)erver or (c)lient? ";

    char choice;

    std::cin >> choice;

    if (choice == 's' || choice == 'S')
    {
        std::cout << "Port: ";

        int port;
        std::cin >> port;

        auto& server = MultiVoxel::Server::ServerBase::GetInstance();

        if (!server.Initialize(port))
            return EXIT_FAILURE;

        auto& temp = MultiVoxel::Independent::Core::Settings::GetInstance().REPLICATION_SENDER.Get();

        server.RegisterPacketSender(&temp);

        auto a = PingSender();

        //server.RegisterPacketSender(&a);

        server.Run();
    }
    else
    {
        std::cout << "Server address: ";

        std::string address;
        std::cin >> address;

        std::cout << "Port: ";

        int port;
        std::cin >> port;

        auto& client = MultiVoxel::Client::ClientBase::GetInstance();

        if (!client.Initialize(address, port))
            return EXIT_FAILURE;

        auto& temp = MultiVoxel::Independent::Core::Settings::GetInstance().REPLICATION_RECEIVER.Get();

        client.RegisterPacketReceiver(&temp);

        auto a = PingReceiver();

        //client.RegisterPacketReceiver(&a);

        client.Run();
    }

	return 0;
}