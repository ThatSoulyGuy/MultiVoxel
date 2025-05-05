#include "Client/ClientBase.hpp"
#include "Server/ECS/GameObjectManager.hpp"
#include "Server/ServerBase.hpp"

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

        auto server = MultiVoxel::Server::ServerBase::Create(port);

        if (!server)
            return EXIT_FAILURE;

        server->RegisterOnUpdateCallback([&]()
        {
            MultiVoxel::Server::ECS::GameObjectManager::GetInstance().Update();
            MultiVoxel::Server::ECS::GameObjectManager::GetInstance().Render();
        }); //TODO: Make some sort of better registration for this

        server->Run();
    }
    else
    {
        std::cout << "Server address: ";

        std::string addr;
        std::cin >> addr;

        std::cout << "Port: ";

        int port;
        std::cin >> port;

        auto client = MultiVoxel::Client::ClientBase::Create(addr, port);

        if (!client)
            return EXIT_FAILURE;

        client->Run();
    }

	return 0;
}