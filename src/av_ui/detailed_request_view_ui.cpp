#include <av_ui/detailed_request_view_ui.hpp>

namespace avUi
{
    std::optional<int64_t> pendingParamDel;
    std::optional<int64_t> capturedReqId;

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id)
        : avR::UiComponent(std::move(id)), footer_height(-1.f),
          request_storage(std::make_unique<avS::AvRequestStorage>()),
          request_params_storage(std::make_unique<avS::AvRequestParamsStorage>()), response_http_code(0),
          last_status(avNet::response_status::Ok), has_response(false)
    {
        this->window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id, avR::AvState *sharedState) : DetailedRequestViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);

        if (this->shared_state->display_request)
        {
            capturedReqId = this->shared_state->display_request->id;
            this->shared_state->display_request->params =
                this->request_params_storage->select_by_req_id(capturedReqId.value());
        }
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
        // never let the worker outlive this component (and its NetworkManager / response_body).
        if (this->pending_response.valid())
            this->pending_response.wait();
    }

    void DetailedRequestViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_detailed_view)
            return;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const float x = viewport->WorkSize.x;
        const float y = viewport->WorkSize.y;
        ImGui::SetNextWindowPos(ImVec2(x * .2f, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(x * .8f, y), ImGuiCond_Once);

        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_detailed_view, this->window_flags))
        {
            if (!this->shared_state->display_request)
            {
                const char *msg = "No request selected";
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const ImVec2 sz = ImGui::CalcTextSize(msg);
                ImGui::SetCursorPos(ImVec2((avail.x - sz.x) * .5f, (avail.y - sz.y) * .5f));
                ImGui::TextDisabled("%s", msg);
                ImGui::End();
                return;
            }

            // pick up a completed request once per frame so the header (button state) and
            // footer (response) agree within the same frame.
            this->poll_response();

            const ImGuiStyle &style = ImGui::GetStyle();
            const ImVec2 availRegion = ImGui::GetContentRegionAvail();

            if (this->footer_height < 0.f)
                this->footer_height = availRegion.y * .4f;

            const float splitterThikness = 3.f;
            const float minFooter = 50.f;
            const float maxFooter = availRegion.y - 100.f;
            this->footer_height = std::clamp(this->footer_height, minFooter, maxFooter);
            const float headerHeight = (availRegion.y - this->footer_height) * .1f;

            ImGui::BeginChild("header", ImVec2(0, headerHeight));
            this->render_header(style);
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::BeginChild("main_content", ImVec2(0, -(this->footer_height + splitterThikness)));
            this->render_main_content(style);
            ImGui::EndChild();

            ImGui::InvisibleButton("##splitter", ImVec2(-1.f, splitterThikness));
            if (ImGui::IsItemActive())
            {
                this->footer_height -= ImGui::GetIO().MouseDelta.y;
                this->footer_height = std::clamp(this->footer_height, minFooter, maxFooter);
            }
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }

            {
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                float midY = (pMax.y + pMin.y) * .5f;
                ImU32 col = ImGui::GetColorU32(ImGui::IsItemActive()    ? ImGuiCol_SeparatorActive
                                               : ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered
                                                                        : ImGuiCol_Separator);

                ImGui::GetWindowDrawList()->AddLine(ImVec2(pMin.x, midY), ImVec2(pMax.x, midY), col, splitterThikness);
            }

            ImGui::BeginChild("footer", ImVec2(0, 0));
            this->render_footer(style);
            ImGui::EndChild();

            ImGui::ShowDemoWindow();
        }
        ImGui::End();
    }
    void DetailedRequestViewUi::render_header(const ImGuiStyle &style)
    {
        avR::AvRequest &req = *this->shared_state->display_request;
        if (!req.title.has_value())
            req.title.emplace(req.display_name());

        const float rowH = ImGui::GetFrameHeight();
        const float availH = ImGui::GetContentRegionAvail().y;
        if (availH > rowH)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availH - rowH) * 0.5f);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, this->get_method_color(req.method));
        const char *current = avNet::NetworkManager::method_text(req.method);
        ImGui::SetNextItemWidth(ImGui::CalcTextSize(current).x + style.FramePadding.x * 2.0f + ImGui::GetFrameHeight());

        if (ImGui::BeginCombo("##method", current))
        {
            static constexpr avNet::request_method methods[] = {
                avNet::request_method::get,  avNet::request_method::post,  avNet::request_method::put,
                avNet::request_method::del,  avNet::request_method::patch, avNet::request_method::options,
                avNet::request_method::head,
            };

            for (avNet::request_method m : methods)
            {
                const bool selected = (req.method == m);
                ImGui::PushStyleColor(ImGuiCol_Text, this->get_method_color(m));
                if (ImGui::Selectable(avNet::NetworkManager::method_text(m), selected))
                {
                    req.method = m;
                    this->save_state_change();
                }
                ImGui::PopStyleColor();
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);
        if (ImGui::InputText("##title_edit", &req.url,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            this->save_state_change();
        };

        ImGui::SameLine();
        const bool in_flight = this->pending_response.valid();
        ImGui::BeginDisabled(in_flight);
        if (ImGui::Button(in_flight ? "Sending..." : "Send"))
        {
            this->send_request();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", this->root.timestamp_to_date(req.timestamp).c_str());
    }
    void DetailedRequestViewUi::render_main_content(const ImGuiStyle &style)
    {
        if (ImGui::BeginTabBar("req_tabs"))
        {
            if (ImGui::BeginTabItem("params"))
            {
                this->render_tab_params();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("headers"))
            {
                this->render_tab_headers();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("body"))
            {
                this->render_tab_body();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("auth"))
            {
                this->render_tab_auth();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    void DetailedRequestViewUi::render_footer(const ImGuiStyle &style)
    {
        if (this->pending_response.valid())
        {
            ImGui::TextDisabled("Sending request...");
            return;
        }

        if (!this->has_response)
        {
            ImGui::TextDisabled("No response yet - press Send to run the request.");
            return;
        }

        const bool ok = this->last_status == avNet::response_status::Ok;
        const ImVec4 statusColor = ok ? ImVec4(0.40f, 0.80f, 0.40f, 1.f) : ImVec4(0.90f, 0.40f, 0.40f, 1.f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(statusColor, "%s", avNet::NetworkManager::status_text(this->last_status));
        ImGui::SameLine();
        ImGui::TextDisabled("HTTP %ld", this->response_http_code);
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu bytes)", this->response_body.size());
        ImGui::Separator();

        // read-only body view fills the remaining footer space. It wraps at the child's right
        // edge (no horizontal scroll) and only scrolls vertically on overflow.
        if (ImGui::BeginChild("##response_body_view", ImVec2(0, 0)))
        {
            ImGui::PushTextWrapPos(0.f); // 0 == wrap at the child's right edge
            ImGui::TextUnformatted(this->response_body.data(),
                                   this->response_body.data() + this->response_body.size());
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();
    }

    void DetailedRequestViewUi::send_request()
    {
        if (!this->shared_state || !this->shared_state->display_request)
            return;

        // one request at a time; ignore the button while a fetch is in flight.
        if (this->pending_response.valid())
            return;

        avR::AvRequest *req = this->shared_state->display_request;

        // params are the source of truth for the query string: assemble the final url from
        // them, reflect it back into the editable url, and persist.
        std::string url = build_url(req->url, req->params);
        req->url = url;
        this->save_state_change();

        // reset the destination on the UI thread before the worker starts writing to it.
        // std::launch::async establishes a happens-before with the worker; the UI thread only
        // reads response_body / response_http_code again after the future is ready (poll_response).
        this->response_body.clear();
        this->response_http_code = 0;
        this->has_response = false;

        // run off the UI thread so a slow/dead endpoint never freezes the window.
        this->pending_response = std::async(std::launch::async, [this, url = std::move(url)]() {
            return this->network_manager.get(url.c_str(), this->response_body, &this->response_http_code);
        });
    }

    void DetailedRequestViewUi::poll_response()
    {
        if (this->pending_response.valid() &&
            this->pending_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            this->last_status = this->pending_response.get();
            this->has_response = true;
        }
    }

    void DetailedRequestViewUi::save_state_change() const
    {
        this->request_storage->upsert(this->shared_state->display_request);
    }

    // percent-encode per RFC 3986: unreserved chars pass through, everything else -> %XX
    static std::string url_encode(std::string_view s)
    {
        static constexpr char hex[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
                c == '_' || c == '.' || c == '~')
            {
                out.push_back(static_cast<char>(c));
            }
            else
            {
                out.push_back('%');
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 0x0F]);
            }
        }
        return out;
    }

    // The params table is the source of truth for the query string: any existing "?..." on
    // base_url is dropped and rebuilt from the included params; a trailing "#fragment" is kept.
    std::string DetailedRequestViewUi::build_url(std::string_view base_url,
                                                 const std::vector<avR::AvRequestParams> &params)
    {
        std::string_view fragment;
        if (const size_t hash = base_url.find('#'); hash != std::string_view::npos)
        {
            fragment = base_url.substr(hash); // includes the '#'
            base_url = base_url.substr(0, hash);
        }

        if (const size_t query = base_url.find('?'); query != std::string_view::npos)
            base_url = base_url.substr(0, query);

        std::string url(base_url);

        bool first = true;
        for (const avR::AvRequestParams &p : params)
        {
            if (!p.included || p.key.empty())
                continue;

            url.push_back(first ? '?' : '&');
            first = false;

            url += url_encode(p.key);
            url.push_back('=');
            url += url_encode(p.value);
        }

        url.append(fragment);
        return url;
    }
    void DetailedRequestViewUi::render_tab_params() const
    {
        if (capturedReqId.has_value() && capturedReqId != this->shared_state->display_request->id)
        {
            capturedReqId = this->shared_state->display_request->id;
            this->shared_state->display_request->params =
                this->request_params_storage->select_by_req_id(capturedReqId.value());
        }

        avR::UiScopedStyle style(avR::UiScopedStyle::Style{.frame_rounding = 0, .frame_border = 0});

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;
        bool isModified = false;

        if (ImGui::BeginTable("params_table", 5, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("included");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            const size_t count = this->shared_state->display_request->params.size();
            clipper.Begin(count);
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    avR::AvRequestParams *item = &this->shared_state->display_request->params[row_n];
                    ImGui::PushID(item);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::InputText("##key", &item->key,
                                         ImGuiInputTextFlags_EnterReturnsTrue |
                                             ImGuiInputTextFlags_CtrlEnterForNewLine))
                    {
                        isModified = true;
                    }
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::InputText("##val", &item->value,
                                         ImGuiInputTextFlags_EnterReturnsTrue |
                                             ImGuiInputTextFlags_CtrlEnterForNewLine))
                    {
                        isModified = true;
                    }

                    ImGui::TableNextColumn();
                    if (item->editing)
                    {
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (item->set_focus)
                        {
                            ImGui::SetKeyboardFocusHere();
                            item->set_focus = false;
                        }
                        ImGui::InputText("##desc", &item->description);
                        if (ImGui::IsItemDeactivated())
                        {
                            item->editing = false;
                            isModified = true;
                        }
                    }
                    else
                    {
                        ImVec2 start = ImGui::GetCursorPos();
                        float wrap = ImGui::GetContentRegionAvail().x;

                        ImGui::PushTextWrapPos(start.x + wrap);
                        ImGui::TextUnformatted(item->description.empty() ? " " : item->description.c_str());
                        ImGui::PopTextWrapPos();

                        float h = ImGui::GetItemRectSize().y;

                        ImGui::SetCursorPos(start);
                        if (ImGui::InvisibleButton("##desc_click", ImVec2(wrap, h)))
                        {
                            item->editing = true;
                            item->set_focus = true;
                        }
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##included", &item->included))
                        isModified = true;
                    ImGui::TableNextColumn();
                    if (ImGui::Button("delete"))
                    {
                        pendingParamDel = item->id;
                    }
                    ImGui::PopID();
                }
            ImGui::EndTable();
        }

        if (ImGui::Button("add param"))
        {
            this->shared_state->display_request->params.push_back(
                avR::AvRequestParams{.request_id = this->shared_state->display_request->id});
            isModified = true;
        }

        bool paramsChanged = isModified;

        if (isModified)
            this->request_params_storage->upsert(this->shared_state->display_request->params);

        if (pendingParamDel.has_value())
        {
            const int64_t id = pendingParamDel.value();
            this->request_params_storage->del(id);
            std::erase_if(this->shared_state->display_request->params,
                          [id](avR::AvRequestParams &p) { return p.id == id; });
            pendingParamDel.reset();
            paramsChanged = true;
        }

        // params are the source of truth for the query string: reflect any change back into the
        // url shown/edited in the header, and persist it.
        if (paramsChanged)
        {
            avR::AvRequest *req = this->shared_state->display_request;
            req->url = build_url(req->url, req->params);
            this->save_state_change();
        }
    }
    void DetailedRequestViewUi::render_tab_headers() const
    {
    }
    void DetailedRequestViewUi::render_tab_body() const
    {
    }
    void DetailedRequestViewUi::render_tab_auth() const
    {
    }
} // namespace avUi
