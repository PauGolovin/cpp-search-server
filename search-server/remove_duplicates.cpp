#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<int, std::set<std::string>> id_words;
    for (const auto& id : search_server) {
        std::map<std::string, double> words = search_server.GetWordFrequencies(id);
        for (const auto& [word, TF] : words) {
            id_words[id].insert(word);
        }
    }
    for (const auto& [id1, words1] : id_words) {
        for (const auto& [id2, words2] : id_words) {
            if (id1 < id2 && words1 == words2) {
                search_server.RemoveDocument(id2);
            }
        }
    }
}