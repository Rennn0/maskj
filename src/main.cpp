#include <av_ui/network_manager_ui.hpp>

int main()
{
    avR::UiComponent *const networkManager = new avUi::NetworkManagerUi();
    networkManager->draw();
}
