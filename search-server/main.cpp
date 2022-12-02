#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

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
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const string& word : words)
            documents_[word].insert({ document_id, 0 });

        // solve TF
        if (words.size() != 0)
            for (const string& word : words)
                documents_[word][document_id] = 1.0 * count(words.begin(), words.end(), word) / words.size();


        ++document_count_;
    }


    // заходим сюда
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };
    // double is TF
    map<string, map<int, double>> documents_;

    int document_count_ = 0;

    //map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }


    // 2 тут надо создать два множества
    Query ParseQuery(const string& text) const
    {
        Query query_;
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }

        for (const string& word : query_words)
        {
            if (word[0] == '-')
            {
                string minus_word = word.substr(1);
                query_.minus_words.insert(minus_word);
            }
            else
            {
                query_.plus_words.insert(word);
            }
        }

        return query_;
    }

    // fixed and add IDF finder function
    // не вижу смысла в добавлении данной функции:
    // если оставить так как сделано сейчас, то у нас просто увеличивается количество строчек
    // если IDF определять так: id_relevance[id] += TF * IDF_Finder(plus_word)
    // то для одного плюс-слова мы будем рассчитывать IDF несколько раз, и увеличим время выполнения программы
    // могли бы Вы пояснить, чем становится лучше код с добавлением данной функции?
    // заранее спасибо!
    double IDF_Finder(const string& word)
    {
        return log(1.0 * document_count_ / documents_.at(word).size());
    }

    // 3 тут надо получать документы все и подавать 2 множества
    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> id_relevance;


        for (const string& plus_word : query_words.plus_words)
            if (documents_.count(plus_word))
            {
                //double IDF = log(1.0 * document_count_ / documents_.at(plus_word).size());

                // либо так
                double IDF = IDF_Finder(plus_word);
                for (const auto& [id, TF] : documents_.at(plus_word))
                {
                    id_relevance[id] += TF * IDF;

                    // либо так:
                    //id_relevance[id] += TF * IDF_Finder(plus_word);
                }
            }

        for (const string& minus_word : query_words.minus_words)
            if (documents_.count(minus_word))
                for (const auto& [id, relevance] : documents_.at(minus_word))
                    if (id_relevance.count(id))
                        id_relevance.erase(id);


        for (const auto& [id, relevance] : id_relevance)
            matched_documents.push_back({ id, relevance });

        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}