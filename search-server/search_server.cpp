//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎
#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }
void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if (document_id < 0)
    {
        throw std::invalid_argument("Unacceptable id. Id must be greater than zero.");
    }
    if (documents_.count(document_id))
    {
        throw std::invalid_argument("Unacceptable id. This id is already used.");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    index_id.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    std::tuple<std::vector<std::string>, DocumentStatus> result = { matched_words, documents_.at(document_id).status };
    return result;
}

std::vector <int> ::iterator SearchServer::begin(){
    return index_id.begin();
}

std::vector <int> ::iterator SearchServer::end(){
    return index_id.end();
}

const std::map<std::string, double> SearchServer::GetWordFrequencies(int document_id) const {
    std::map<std::string, double> result;
    if (find(index_id.begin(), index_id.end(), document_id) == index_id.end()) {
        return result;
    }
    for (const auto& [word, id_TF] : word_to_document_freqs_) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            result[word] = word_to_document_freqs_.at(word).at(document_id);
        }
    }
    return result;
}

void SearchServer::RemoveDocument(int document_id) {
    auto it = find(index_id.begin(), index_id.end(), document_id);
    if (it == index_id.end()) {
        return;
    }
    index_id.erase(it);
    documents_.erase(document_id);
    for (const auto& [word, map] : word_to_document_freqs_) {
        if (word_to_document_freqs_.at(word).count(document_id)){
            word_to_document_freqs_.at(word).erase(document_id);
            /*
            if (word_to_document_freqs_.at(word).empty()) {
                word_to_document_freqs_.erase(word);
            }*/
        }
    }
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Unacceptable symbols in word \"" + word + "\".");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Unacceptable symbols in word \"" + word + "\".");
        }
        if (word[0] == '-' && word[1] == '-') {
            throw std::invalid_argument("Error in the word \"" + word +
                "\". You should use just one minus to mark minus-word.");
        }
        if (word == "-") {
            throw std::invalid_argument("Unacceptable word \"" + word + "\".");
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
