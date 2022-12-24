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
        // если не существует, то просто добавляем
        if (!id_words.count(docs_words)) {
            id_words[docs_words] = id;
        }
        // условие, что остается документ с меньшим id
        else if (id_words.at(docs_words) > id) {
            id_to_remove.push_back(id_words.at(docs_words));
            id_words[docs_words] = id;
        }
        // если оба из условий не удовлетворены, то удаляем этот документ
        else {
            id_to_remove.push_back(id);
        }
    }
    // удаляем дубликаты
    for (const int id : id_to_remove) {
        search_server.RemoveDocument(id);
    }
}