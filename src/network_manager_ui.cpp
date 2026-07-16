#include <av_ui/network_manager_ui.hpp>

// Including <GLFW/glfw3.h> without GLFW_INCLUDE_NONE also pulls in the platform
// OpenGL header, giving us the GL 1.1 calls (glViewport/glClear/...) we use for
// the frame clear — portably across Windows/Linux/macOS.

namespace avUi
{
    const char *status_text(avNet::response_status s)
    {
        switch (s)
        {
        case avNet::response_status::Ok:
            return "Ok";
        case avNet::response_status::Failed:
            return "Failed";
        case avNet::response_status::Canceled:
            return "Canceled";
        }
        return "Unknown";
    }

    NetworkManagerUi::NetworkManagerUi() : m_root("NetworkManagerUI")
    {
        if (!glfwInit())
        {
            const char *desc;
            int code = glfwGetError(&desc);
            this->m_root.log_error(desc);
            throw std::runtime_error("glfw init failed");
        }
    }

    NetworkManagerUi::~NetworkManagerUi()
    {
        glfwTerminate();
    }

    int NetworkManagerUi::start() const
    {
        return 0;
    }

}
