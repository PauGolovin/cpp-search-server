#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double Divergence = 1e-6;

// считываем строку
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

// считываем число
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

// делим строку на слова
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
    // заполняем стоп слова
    void SetStopWords(const string& text) {
        // Ваша реализация данного метода
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    // добавляем документы
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        // Ваша реализация данного метода
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    // general method
    template<typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(), CheckDivergence);
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    // method for raw_query
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }

    //method for raw_query and status
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus input_status) const {
        return FindTopDocuments(raw_query, [&input_status](int document_id, DocumentStatus status, int rating) { return status == input_status; });
    }

    // количество документов
    int GetDocumentCount() const {
        // Ваша реализация данного метода
        return documents_.size();
    }

    // Проверка соответствия документа запросу
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        // Ваша реализация данного метода
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    // Проверка пограшности в релевантности
    static bool CheckDivergence(const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < Divergence) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
    }

    // Проверка слова на стоп-слово
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // Убираем из текста стоп-слова
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // Расчет рейтинга документа
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

        return rating_sum / static_cast<int>(ratings.size());
    }

    // структура для определения слова
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // Парсим каждое слово запроса в плюс- и минус-слова
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    // структура запроса в плюс- и минус-словах
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    // Парсим запрос в плюс- и минус-слова
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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

    // Ищем IDF
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // ядро, ищем документы
    template<typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                // тут проверка по фильтрации будет
                if (key_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};


//-------------------TestFramework_START-----------------
template<typename Term1, typename Term2>
ostream& operator<<(ostream& out, pair<Term1, Term2> pair_) {
    out << pair_.first << ": "s << pair_.second;
    return out;
}

template <typename Term>
void Print(ostream& out, const Term& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}

template<typename Term>
ostream& operator<< (ostream& out, const vector<Term>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}


template<typename Term>
ostream& operator<< (ostream& out, const set<Term>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template<typename Term1, typename Term2>
ostream& operator<< (ostream& out, const map<Term1, Term2>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(TestFunc test_func, const string& test_name) {
    test_func();
    cerr << test_name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)


//-------------------TestFramework_END-------------------


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
// Тест матчинга и вычленения стоп- и минус-слов из документа
void TestMatchingStopMinusWords() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    // тест матчинга
    {
        const auto [words, status] = server.MatchDocument("white cat"s, 42);
        ASSERT_EQUAL(words.size(), 1u);
        ASSERT(status == DocumentStatus::ACTUAL);

    }
    // тест стоп-слов
    {
        server.SetStopWords("cat"s);
        const auto [words, status] = server.MatchDocument("white cat"s, 42);
        ASSERT_EQUAL(words.size(), 0u);
    }
    // тест минус-слов
    {
        const auto [words, status] = server.MatchDocument("white -cat"s, 42);
        ASSERT_EQUAL(words.size(), 0u);
    }
}
// тест сортировки по релевантности и рейтинга
void TestRelevanceRating() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, { 2, 3, 4 });
    server.AddDocument(44, "dog in the park"s, DocumentStatus::ACTUAL, { 3, 4, 5 });

    const auto found_docs = server.FindTopDocuments("cat in the city"s);
    // тест рейтинга
    ASSERT_EQUAL(found_docs[0].rating, 2u);
    ASSERT_EQUAL(found_docs[1].rating, 3u);
    ASSERT_EQUAL(found_docs[2].rating, 4u);
    // тест сортировки по релевантности
    for (int i = 0; i < found_docs.size() - 1; ++i)
        ASSERT_HINT(found_docs[i].relevance > found_docs[i + 1].relevance,
            "Docs must be sorted by relevance"s);
    // тест релевантности
    double relevance0 = log(3.0) * 0.25 + log(1.5) * 0.25;
    double relevance1 = log(1.5) * 0.25;
    double relevance2 = 0;
    ASSERT_EQUAL_HINT(found_docs[0].relevance, relevance0, "Incorrect relevance calculating."s);
    ASSERT_EQUAL_HINT(found_docs[1].relevance, relevance1, "Incorrect relevance calculating."s);
    ASSERT_EQUAL_HINT(found_docs[2].relevance, relevance2, "Incorrect relevance calculating."s);
}
// тест функции-предиката и нахождения по статусу
void TestPredicateStatus() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(43, "dog in the city"s, DocumentStatus::BANNED, { 2, 3, 4 });
    server.AddDocument(44, "dog in the park"s, DocumentStatus::REMOVED, { 3, 4, 5 });
    server.AddDocument(46, "dog in the park"s, DocumentStatus::IRRELEVANT, { 3, 4, 5 });
    // тест поиска по статусу
    {
        // поиск REMOVED
        const auto found_docs_REMOVED = server.FindTopDocuments("cat in the city"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs_REMOVED.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs_REMOVED[0].id, 44u, "Docs must be sorted by status"s);
        // поиск ACTUAL
        const auto found_docs_ACTUAL = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs_ACTUAL.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs_ACTUAL[0].id, 42u, "Docs must be sorted by status"s);
        // поиск BANNED
        const auto found_docs_BANNED = server.FindTopDocuments("cat in the city"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs_BANNED.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs_BANNED[0].id, 43u, "Docs must be sorted by status"s);
        // поиск IRRELEVANT
        const auto found_docs_IRRELEVANT = server.FindTopDocuments("cat in the city"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs_IRRELEVANT.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs_IRRELEVANT[0].id, 46u, "Docs must be sorted by status"s);
    }
    // тест предиката 1
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs[0].id, 43u, "Docs must be sorted by id"s);
    }
    // тест предиката 2
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, [](int document_id, DocumentStatus status, int rating) { return rating < 3; });
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs[0].id, 42u, "Docs must be sorted by rating"s);
    }
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestMatchingStopMinusWords);
    RUN_TEST(TestRelevanceRating);
    RUN_TEST(TestPredicateStatus);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}