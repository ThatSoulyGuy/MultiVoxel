#include <iostream>
#include <GameNetworkingSockets/steamclientpublic.h>

int main()
{
	SteamIPAddress_t ip;

	ip.IPv4Any();

	std::cout << "Hello CMake." << std::endl;
	return 0;
}