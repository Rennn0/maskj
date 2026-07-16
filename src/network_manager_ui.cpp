#include <av_ui/network_manager_ui.hpp>
#include <av_root/av_button.hpp>
#include <av_root/av_custom.hpp>
#include <av_root/av_div.hpp>

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

    const char *method_text(avNet::request_method m)
    {
        switch (m)
        {
        case avNet::request_method::get:
            return "GET";
        case avNet::request_method::post:
            return "POST";
        }
        return "Unknown";
    }

    NetworkManagerUi::NetworkManagerUi() : avRoot("NetworkManagerUI"),
                                           window(nullptr),
                                           width(0),
                                           height(0)
    {
        if (!glfwInit())
        {
            const char *desc;
            glfwGetError(&desc);
            this->avRoot.log_error(desc);
            throw std::runtime_error("glfw init failed");
        }

        this->monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        // glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        this->width = mode->width;
        this->height = mode->height;
    }

    NetworkManagerUi::~NetworkManagerUi()
    {
        this->avRoot.log_info("dctor");
        glfwTerminate();
        this->window = nullptr;
        this->monitor = nullptr;
    }

    void NetworkManagerUi::draw()
    {
        this->window = glfwCreateWindow(this->width, this->height, "Arvis", this->monitor, nullptr);
        glfwMakeContextCurrent(this->window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(this->window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        std::shared_ptr<avR::UiComponent> root = std::make_shared<avR::AvDiv>("root");
        this->setup_root(root);

        while (!glfwWindowShouldClose(this->window))
        {
            glfwPollEvents();
            this->check_keyboard_events();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            const ImGuiViewport *viewPort = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewPort->WorkPos);
            ImGui::SetNextWindowSize(viewPort->WorkSize);
            const ImGuiWindowFlags hostFlag = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("##host", nullptr, hostFlag);
            ImGui::PopStyleVar();
            root->draw();
            ImGui::End();

            ImGui::Render();
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.07f, 0.07f, 0.09f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
    }

    void NetworkManagerUi::check_keyboard_events()
    {
        if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(this->window, true);
        }
    }

    void NetworkManagerUi::setup_root(std::shared_ptr<avR::UiComponent> &root)
    {
        using Dir = avR::AvDiv::Direction;

        {
            avR::AvDiv::Config conf;
            conf.direction = Dir::Vertical;
            conf.size = ImVec2(0, 0);
            conf.padding = ImVec2(30, 10);
            conf.spacing = 20.f;
            conf.border = false;
            std::static_pointer_cast<avR::AvDiv>(root)->configure(conf);
        }

        {
            avR::AvDiv::Config conf;
            conf.direction = Dir::Horizontal;
            conf.size = ImVec2(0, 60);
            conf.background = ImVec4(0.05f, 0.08f, 0.18f, 1.0f);
            conf.border = true;
            avR::UiComponent *header = root->add_child(std::make_unique<avR::AvDiv>("header", conf));

            avR::AvButton::Style style;
            style.size = ImVec2(140, 36);
            style.background = ImVec4(0.13f, 0.45f, 0.90f, 1.0f);
            style.hovered = ImVec4(0.20f, 0.55f, 1.00f, 1.0f);
            style.active = ImVec4(0.10f, 0.38f, 0.80f, 1.0f);
            style.rounding = 6.f;
            avR::UiComponent *addBtn =
                header->add_child(std::make_unique<avR::AvButton>("add request", style));
            addBtn->set_on_click([this]
                                 {
                                     AvRequest req;
                                     req.id = this->nextRequestId++;
                                     this->requests.push_back(std::move(req));
                                     // focus the new entry right away
                                     this->selectedRequest = static_cast<int>(this->requests.size()) - 1;
                                 });
        }

        {
            avR::AvDiv::Config conf;
            conf.direction = Dir::Horizontal;
            conf.size = ImVec2(0, 0);
            conf.padding = ImVec2(0, 0);
            conf.spacing = 0.f;
            conf.resizable = true;         // draggable divider between sidebar and mid
            conf.splitter_thickness = 6.f;
            conf.resize_min = 150.f;       // sidebar can't shrink below this
            conf.resize_max = 500.f;       // ...or grow past this
            avR::UiComponent *body = root->add_child(std::make_unique<avR::AvDiv>("body", conf));

            conf = {};
            conf.direction = Dir::Vertical;
            conf.size = ImVec2(220, 0);
            conf.border = true;
            avR::UiComponent *sidebar = body->add_child(std::make_unique<avR::AvDiv>("sidebar", conf));
            // Data-driven list: re-rendered from `requests` every frame, so entries
            // added by the header button show up without touching the tree.
            sidebar->add_child(std::make_unique<avR::AvCustom>(
                "request_list", [this]
                {
                    ImGui::TextDisabled("requests");
                    ImGui::Separator();
                    for (int i = 0; i < static_cast<int>(this->requests.size()); ++i)
                    {
                        ImGui::PushID(i);
                        const std::string label = this->requests[i].display_name();
                        if (ImGui::Selectable(label.c_str(), this->selectedRequest == i))
                        {
                            this->selectedRequest = i;
                        }
                        ImGui::PopID();
                    }
                    if (this->requests.empty())
                    {
                        ImGui::TextDisabled("(empty — press 'add request')");
                    }
                }));

            conf = {};
            conf.size = ImVec2(0, 0);
            conf.background = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
            avR::UiComponent *mid = body->add_child(std::make_unique<avR::AvDiv>("mid", conf));
            // Detail pane for whichever request is selected in the sidebar.
            mid->add_child(std::make_unique<avR::AvCustom>(
                "request_details", [this]
                {
                    if (this->selectedRequest < 0 ||
                        this->selectedRequest >= static_cast<int>(this->requests.size()))
                    {
                        ImGui::TextDisabled("select a request from the sidebar");
                        return;
                    }

                    const AvRequest &req = this->requests[this->selectedRequest];
                    ImGui::Text("%s", req.display_name().c_str());
                    ImGui::Separator();
                    ImGui::Text("id:     %d", req.id);
                    ImGui::Text("method: %s", method_text(req.method));
                    ImGui::Text("url:    %s", req.url.c_str());
                    ImGui::Text("body:   %s", req.body.empty() ? "(empty)" : req.body.c_str());
                }));
        }
    }
}
