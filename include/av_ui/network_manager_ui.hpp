#pragma once
#include <iostream>
#include <chrono>
#include <future>
#include <string>
#include <vector>
#include <av_net/network_manager.hpp>
#include <av_root/root.hpp>
#include <av_root/ui_component.hpp>
#include <av_ui/av_request.hpp>

namespace avUi
{
    class NetworkManagerUi : public avR::UiComponent
    {
    public:
        NetworkManagerUi();
        ~NetworkManagerUi();

        /// @brief Opens the arvis GUI window and runs the event/render loop until
        ///        the user closes the window.
        /// @return process exit code (0 on clean exit, non-zero on init failure).
        void draw() override;

    private:
        int width;
        int height;
        avR::AvRoot avRoot;
        GLFWwindow *window;
        GLFWmonitor *monitor;

        std::vector<AvRequest> requests; ///< user-created requests (sidebar list)
        int selectedRequest = -1;        ///< index into requests, -1 = none
        int nextRequestId = 1;           ///< sequential id for display names

        void check_keyboard_events();
        void setup_root(std::shared_ptr<avR::UiComponent> &root);
    };
}
