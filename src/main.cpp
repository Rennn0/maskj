#include <mj_net/network_manager.hpp>

int main()
{
    const mjNet::NetworkManager *networkManagerPtr = new mjNet::NetworkManager();
    const mjNet::response_status r1 = networkManagerPtr->get("https://api.xati.org/health");
    const mjNet::response_status r2 = networkManagerPtr->get("https://www.xati.org/18");

    delete networkManagerPtr;
    return 0;
}