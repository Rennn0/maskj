#pragma once
#include <iostream>
#include <chrono>
#include <future>
#include <string>
#include <av_net/network_manager.hpp>
#include <av_root/root.hpp>
#include <av_root/ui_component.hpp>

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
        int m_width;
        int m_height;
        avR::AvRoot m_root;
        GLFWwindow *m_window;
    };
}
