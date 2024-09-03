// Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».
#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                          { return document_status == status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}
int RequestQueue::GetNoResultRequests() const
{
    // напишите реализацию
    return (min_in_day_ + 1) - RequestQueue::count_find_docs_;
}