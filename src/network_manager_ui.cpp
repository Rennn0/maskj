#include <av_ui/network_manager_ui.hpp>
#include <av_root/av_button.hpp>

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

    NetworkManagerUi::NetworkManagerUi() : m_root("NetworkManagerUI"),
                                           m_window(nullptr),
                                           m_width(600),
                                           m_height(700)
    {
        if (!glfwInit())
        {
            const char *desc;
            glfwGetError(&desc);
            this->m_root.log_error(desc);
            throw std::runtime_error("glfw init failed");
        }
    }

    NetworkManagerUi::~NetworkManagerUi()
    {
        glfwTerminate();
        m_window = nullptr;
    }

    void NetworkManagerUi::draw()
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_window = glfwCreateWindow(m_width, m_height, "Arvis", nullptr, nullptr);
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        std::shared_ptr<avR::UiComponent> root = std::make_shared<avR::AvButton>("test me");
        root->set_on_click([btn = root.get()]
                           { btn->log_info("hi there"); });

        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Arvis");
            root->draw();
            ImGui::End();

            ImGui::Render();
            int w, h;
            glfwGetFramebufferSize(m_window, &w, &h);
            glViewport(0, 0, w, h);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(m_window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
    }
}
