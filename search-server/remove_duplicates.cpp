#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::set<std::string>, int> id_words;
    std::vector<int> id_to_remove;
    for (const auto& id : search_server) {
        std::map<std::string, double> words = search_server.GetWordFrequencies(id);
        std::set<std::string> docs_words;
        for (const auto& [word, TF] : words) {
            docs_words.insert(word);
        }
        if (!id_words.count(docs_words)) {
            id_words[docs_words] = id;
        }
        else {
            id_to_remove.push_back(id);
        }
    }
    for (const int id : id_to_remove) {
        search_server.RemoveDocument(id);
    }
}