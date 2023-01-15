#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}
SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(std::string_view{ stop_words_text }))
{
}
void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, 
    const std::vector<int>& ratings) {
    if (document_id < 0)
    {
        throw std::invalid_argument("Unacceptable id. Id must be greater than zero.");
    }
    if (documents_.count(document_id))
    {
        throw std::invalid_argument("Unacceptable id. This id is already used.");
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        std::string str_word{ word };
        if (!server_words_.count(str_word)) {
            server_words_[str_word].first = str_word;
            std::string_view ptr_word{ server_words_.at(str_word).first };
            server_words_.at(str_word).second = ptr_word;
        }
        word_to_document_freqs_[server_words_.at(str_word).second][document_id] += inv_word_count;
        id_words_freqs_[document_id][server_words_.at(str_word).second] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    index_id_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, 
    int document_id) const {
    //-------------------------------------------
    const Query query = ParseQuery(raw_query, false);
    std::vector<std::string_view> matched_words;
    if (!index_id_.count(document_id)) {
        return { {}, {} };
    }
    for (std::string_view word : query.plus_words) {
        if (id_words_freqs_.at(document_id).count(word)) {
            matched_words.push_back(word);
        }
    }
    for (std::string_view word : query.minus_words) {
        if (id_words_freqs_.at(document_id).count(word)) {
            matched_words.clear();
            break;
        }
    }
    std::tuple<std::vector<std::string_view>, DocumentStatus> result = { matched_words, documents_.at(document_id).status };
    return result;
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
    std::string_view raw_query, int document_id) const {
    //-------------------------------------------
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
    std::string_view raw_query, int document_id) const {
    //-------------------------------------------
    const Query query = ParseQuery(raw_query, true);
    if (!index_id_.count(document_id)) {
        return { {}, {} };
    }
    if (std::any_of(std::execution::par,
        query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view str)
        { return id_words_freqs_.at(document_id).count(str); })) {
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it_end = std::copy_if(std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [&](std::string_view str) {
            return id_words_freqs_.at(document_id).count(str);
        });
    std::sort(std::execution::par, matched_words.begin(), it_end);
    it_end = std::unique(std::execution::par, matched_words.begin(), it_end);
    matched_words.erase(it_end, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

std::set <int> ::iterator SearchServer::begin() {
    return index_id_.begin();
}

std::set <int> ::iterator SearchServer::end() {
    return index_id_.end();
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    if (index_id_.count(document_id)) {
        return id_words_freqs_.at(document_id);
    }
    std::map<std::string_view, double> empty_result;
    return empty_result;
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    if (!index_id_.count(document_id)) {
        return;
    }
    std::vector<std::string_view> words(id_words_freqs_.at(document_id).size());
    std::transform(std::execution::seq,
        id_words_freqs_.at(document_id).begin(), id_words_freqs_.at(document_id).end(),
        words.begin(),
        [](auto& word_freq) {
            return word_freq.first;
        }
    );
    std::for_each(std::execution::seq,
        words.begin(), words.end(),
        [&](std::string_view word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });
    index_id_.erase(document_id);
    documents_.erase(document_id);
    id_words_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (!index_id_.count(document_id)) {
        return;
    }
    std::vector<std::string_view> words(id_words_freqs_.at(document_id).size());
    std::transform(std::execution::par,
        id_words_freqs_.at(document_id).begin(), id_words_freqs_.at(document_id).end(),
        words.begin(),
        [](auto& word_freq) {
            return word_freq.first;
        }
    );

    std::for_each(std::execution::par,
        words.begin(), words.end(),
        [&](std::string_view word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });
    index_id_.erase(document_id);
    documents_.erase(document_id);
    id_words_freqs_.erase(document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Unacceptable symbols in word \"" + std::string{ word } + "\".");
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool skip_sort) const {
    SearchServer::Query query;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Unacceptable symbols in word \"" + std::string{word} + "\".");
        }
        if (word[0] == '-' && word[1] == '-') {
            throw std::invalid_argument("Error in the word \"" + std::string{ word } +
                "\". You should use just one minus to mark minus-word.");
        }
        if (word == "-") {
            throw std::invalid_argument("Unacceptable word \"" + std::string{ word } + "\".");
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!skip_sort) {
        std::sort(query.minus_words.begin(), query.minus_words.end());
        auto it_minus = std::unique(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(it_minus, query.minus_words.end());

        std::sort(query.plus_words.begin(), query.plus_words.end());
        auto it_plus = std::unique(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(it_plus, query.plus_words.end());
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}