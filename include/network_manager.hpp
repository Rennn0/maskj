#pragma once
/// @brief abstractions over some http methods, uses libcurl
class NetworkManager
{
public:
    /// @brief global init
    NetworkManager();

    /// @brief global cleanup
    ~NetworkManager();

    /// @brief sends get request and writes response data into folder
    /// @param url
    void get(const char *url) const;

private:
    /// @brief
    /// @param str
    void print_info(const char *str) const;

    /// @brief
    /// @param str
    void print_error(const char *str) const;
};