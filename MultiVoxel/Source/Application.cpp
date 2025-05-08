#include "Client/ClientApplication.hpp"
#include "Client/ClientBase.hpp"
#include "Independent/Core/Settings.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"
#include "Server/ServerApplication.hpp"

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

        MultiVoxel::Server::ServerApplication::GetInstance();

        auto& server = MultiVoxel::Server::ServerBase::GetInstance();

        if (!server.Initialize(port))
            return EXIT_FAILURE;

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

        MultiVoxel::Client::ClientApplication::GetInstance();

        auto& client = MultiVoxel::Client::ClientBase::GetInstance();

        if (!client.Initialize(address, port))
            return EXIT_FAILURE;

        client.Run();
    }

	return 0;
}