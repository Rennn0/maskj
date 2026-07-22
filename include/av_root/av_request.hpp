#pragma once

#include <optional>
#include <string>
#include <av_net/network_manager.hpp>
#include <vector>

namespace avR
{
    struct AvRequestParams
    {
        bool included = true;
        bool editing = false;
        bool set_focus = false;
        int64_t id = 0;
        int64_t request_id = 0;
        int64_t order_by = 0;
        std::string key;
        std::string value;
        std::string description;
    };

    /// @brief One user-created request entry (the app-side model the UI lists,
    ///        selects and displays; sending it is wired up later).
    struct AvRequest
    {
        int64_t id = 0;
        int64_t timestamp = 0;
        int64_t order_by = 0;
        avNet::request_method method = avNet::request_method::get;
        std::string url = "https://example.com";
        std::vector<AvRequestParams> params;
        std::optional<std::string> body;
        std::optional<std::string> title;
        std::optional<int> status_code;
        std::optional<std::string> collection;
        const std::string display_name() const { return title.value_or("request#" + std::to_string(id)); }
    };

} // namespace avR
