#include <av_s/av_request_params_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvRequestParamsStorage::AvRequestParamsStorage() : AvStorage()
    {
        this->db->exec(this->create_request_params_table_sql);
    }

    AvRequestParamsStorage::~AvRequestParamsStorage()
    {
    }

    void AvRequestParamsStorage::del(int64_t id) const
    {
        SQLite::Statement q(*this->db, this->delete_request_param_sql);
        q.bind(this->pcol_id, id);
        q.exec();
    }
    void AvRequestParamsStorage::upsert(avR::AvRequestParam &requestParam) const
    {
        SQLite::Statement q(*this->db, this->upsert_request_param_sql);
        bool isInsert = requestParam.id == 0;
        if (isInsert)
            q.bind(this->pcol_id);
        else
            q.bind(this->pcol_id, requestParam.id);

        q.bind(pcol_request_id, requestParam.request_id);
        q.bind(pcol_included, requestParam.included);
        q.bind(pcol_key, requestParam.key);
        q.bind(pcol_value, requestParam.value);
        q.bind(pcol_description, requestParam.description);
        q.bind(pcol_order_by, requestParam.order_by);
        q.exec();
        if (isInsert)
            requestParam.id = this->db->getLastInsertRowid();
    }
    void AvRequestParamsStorage::upsert(std::vector<avR::AvRequestParam> &requestParams) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (avR::AvRequestParam &req : requestParams)
            this->upsert(req);
        tran.commit();
    }

    std::vector<avR::AvRequestParam> AvRequestParamsStorage::select_by_req_id(int64_t requestId) const
    {
        std::vector<avR::AvRequestParam> res;
        SQLite::Statement q(*this->db, this->select_all_request_param_sql);
        q.bind(1, requestId);
        while (q.executeStep())
        {
            avR::AvRequestParam p;
            p.id = q.getColumn(this->pcol_id - 1).getInt64();
            p.request_id = q.getColumn(this->pcol_request_id - 1).getInt64();
            p.included = q.getColumn(this->pcol_included - 1).getInt() == 1;
            p.key = q.getColumn(this->pcol_key - 1).getText();
            p.value = q.getColumn(this->pcol_value - 1).getText();
            p.description = q.getColumn(this->pcol_description - 1).getText();
            p.order_by = q.getColumn(this->pcol_order_by - 1).getInt();
            res.push_back(p);
        }
        return res;
    }
} // namespace avS
