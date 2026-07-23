#include <av_s/av_request_headers_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvRequestHeadersStorage::AvRequestHeadersStorage() : AvStorage()
    {
        this->db->exec(this->create_request_headers_table_sql);
    }

    AvRequestHeadersStorage::~AvRequestHeadersStorage()
    {
    }

    void AvRequestHeadersStorage::del(int64_t id) const
    {
        SQLite::Statement q(*this->db, this->delete_request_header_sql);
        q.bind(this->hcol_id, id);
        q.exec();
    }
    void AvRequestHeadersStorage::upsert(avR::AvRequestHeader &requestHeader) const
    {
        SQLite::Statement q(*this->db, this->upsert_request_header_sql);
        bool isInsert = requestHeader.id == 0;
        if (isInsert)
            q.bind(this->hcol_id);
        else
            q.bind(this->hcol_id, requestHeader.id);

        q.bind(hcol_request_id, requestHeader.request_id);
        q.bind(hcol_included, requestHeader.included);
        q.bind(hcol_key, requestHeader.key);
        q.bind(hcol_value, requestHeader.value);
        q.bind(hcol_description, requestHeader.description);
        q.bind(hcol_order_by, requestHeader.order_by);
        q.exec();
        if (isInsert)
            requestHeader.id = this->db->getLastInsertRowid();
    }
    void AvRequestHeadersStorage::upsert(std::vector<avR::AvRequestHeader> &requestHeaders) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (avR::AvRequestHeader &req : requestHeaders)
            this->upsert(req);
        tran.commit();
    }

    std::vector<avR::AvRequestHeader> AvRequestHeadersStorage::select_by_req_id(int64_t requestId) const
    {
        std::vector<avR::AvRequestHeader> res;
        SQLite::Statement q(*this->db, this->select_all_request_header_sql);
        q.bind(1, requestId);
        while (q.executeStep())
        {
            avR::AvRequestHeader p;
            p.id = q.getColumn(this->hcol_id - 1).getInt64();
            p.request_id = q.getColumn(this->hcol_request_id - 1).getInt64();
            p.included = q.getColumn(this->hcol_included - 1).getInt() == 1;
            p.key = q.getColumn(this->hcol_key - 1).getText();
            p.value = q.getColumn(this->hcol_value - 1).getText();
            p.description = q.getColumn(this->hcol_description - 1).getText();
            p.order_by = q.getColumn(this->hcol_order_by - 1).getInt();
            res.push_back(p);
        }
        return res;
    }
} // namespace avS
