#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> found_docs(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), found_docs.begin(),
        [&search_server](std::string query) { return search_server.FindTopDocuments(query); });
    return found_docs;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> prev = ProcessQueries(search_server, queries);
    std::list<Document> result;
    for (auto& vec_doc : prev) {
        for (auto& doc : vec_doc) {
            result.push_back(std::move(doc));
        }
    }
    return result;
}