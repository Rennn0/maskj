#include <av_root/root.hpp>
#include <chrono>
namespace avR
{
    AvRoot::AvRoot(std::string id) : id(std::move(id))
    {
    }

    AvRoot::~AvRoot()
    {
    }

    void AvRoot::log_info(const std::string_view msg) const noexcept
    {
        this->log_core(std::cout, "info", msg);
    }

    void AvRoot::log_error(const std::string_view msg) const noexcept
    {
        this->log_core(std::cerr, "error", msg);
    }

    void AvRoot::log_core(std::ostream &out, std::string_view level, std::string_view msg) const noexcept
    {
        out << '[' << level << ']' << '(' << this->id << ')' << msg << std::endl;
    }

    const std::string &AvRoot::get_id() const noexcept
    {
        return this->id;
    }

    bool AvRoot::is_today(int64_t timestamp) const
    {
        using namespace std::chrono;
        const auto tp = sys_time<milliseconds>{milliseconds{timestamp}};
        const auto req_day = floor<days>(tp);
        const auto today = floor<days>(system_clock::now());
        return req_day == today;
    }
} // namespace avR
