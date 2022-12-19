#pragma once
#include <deque>
#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // оставил эту функцию тут, так как шаблонная, вроде верно...
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
       // напишите реализацию
        std::vector<Document> Results = search_server_.FindTopDocuments(raw_query, document_predicate);
        RequestQueue::QueryResult query_result;
        query_result.ResultCount = Results.size();
            if (requests_.size() < min_in_day_) {
                requests_.push_back(query_result);
                if (query_result.ResultCount == 0) {
                    ++NoResultRequests_;
                }
            }
            else {
                if (requests_.front().ResultCount == 0) {
                    --NoResultRequests_;
                }
                requests_.pop_front();
                requests_.push_back(query_result);
                if (query_result.ResultCount == 0) {
                    ++NoResultRequests_;
                }
            }
        return Results;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const {
    return RequestQueue::NoResultRequests_;
}
    
private:
    struct QueryResult {
        int ResultCount;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int NoResultRequests_ = 0;
};