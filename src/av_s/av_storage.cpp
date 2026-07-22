#include <av_s/av_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvStorage::AvStorage()
        : db(std::make_unique<SQLite::Database>("_av_.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
    {
    }

    AvStorage::~AvStorage()
    {
    }

    int AvStorage::get_schema_version() const
    {
        SQLite::Statement q(*this->db.get(), "PRAGMA user_version");
        q.executeStep();

        return q.getColumn(0).getInt();
    }
    void AvStorage::set_schema_version(int version) const
    {
        this->db->exec("PRAGMA user_version=" + std::to_string(version));
    }
} // namespace avS
