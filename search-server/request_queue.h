#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate)
    {
        std::vector<Document> f_doc = server_.FindTopDocuments(raw_query, document_predicate);
        requests_.push_back({raw_query, static_cast<int>(f_doc.size())});

        if (min_in_day_ < requests_.size())
        {
            count_find_docs_ -= requests_.front().count;
            requests_.pop_front();
        }
        else
        {
            count_find_docs_ += requests_.back().count;
        }
        return f_doc;
    }
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        std::string query;
        int count;
    };
    std::deque<QueryResult> requests_;
    const static size_t min_in_day_ = 1440;
    const SearchServer &server_;
    int count_find_docs_ = 0;
};
