#pragma once
#include <iostream>
#include <string>
#include <string_view>

namespace avR
{
    class AvRoot
    {
        public:
        explicit AvRoot(std::string name);
        ~AvRoot();

        void log_info(const std::string_view msg)const noexcept;
        void log_error(const std::string_view msg)const noexcept;
        const std::string& get_name()const noexcept;

        private:
        void log_core(std::ostream& out, std::string_view level, std::string_view msg)const noexcept;
        std::string name;
    };
} // namespace avR

