#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EXP = numeric_limits<double>::epsilon();

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating)
    {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    SearchServer() = default;

    template <class T>
    explicit SearchServer(const T &stop_words)
    {
        if (all_of(stop_words.begin(), stop_words.end(), IsValidWord))
        {
            string stop_words_text;
            for (const auto &text : stop_words)
            {
                stop_words_text += " "s;
                stop_words_text += text;
            }
            for (const auto &word : SplitIntoWords(stop_words_text))
            {
                stop_words_.insert(word);
            }
        }
        else
        {
            throw invalid_argument("a special symbol"s);
        }
    }

    explicit SearchServer(const string &stop_words_text)
        : SearchServer(
              SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings)
    {
        if (document_id < 0)
        {
            throw invalid_argument("id < 0"s);
        }
        if (documents_.count(document_id))
        {
            throw invalid_argument("id already exists"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);

        auto tmp = 1.0 / static_cast<double>(words.size());
        for (auto &word : words)
        {
            word_to_document_freqs_[word][document_id] += tmp;
        }

        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});

        documents_index_.push_back(document_id);
    }

    template <class T>
    vector<Document> FindTopDocuments(const string &raw_query, T func) const
    {
        Query query = ParseQuery(raw_query);
        vector<Document> matched_documents = FindAllDocuments(query, func);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < EXP)
                 {
                     return lhs.rating > rhs.rating;
                 }
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus status)
    {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus stat, int rating)
                                { return stat == status; });
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating)
                                { return status == DocumentStatus::ACTUAL; });
    }

    int GetDocumentId(int index) const
    {
        return documents_index_.at(index);
    }

    int GetDocumentCount() const
    {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
                                                        int document_id) const
    {
        Query query = ParseQuery(raw_query);

        vector<string> matched_words;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_index_;

    static bool IsValidWord(const string &word)
    {
        return none_of(word.begin(), word.end(), [](char c)
                       { return c >= '\0' && c < ' '; });
    }

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsValidWord(word))
            {
                throw invalid_argument("unacceptable symbols"s);
            }

            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }

        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        QueryWord result;

        bool is_minus = false;
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        if (text[0] == '-')
        {
            throw invalid_argument("more than one minus sign");
        }
        if (text.empty())
        {
            throw invalid_argument("no text after minus sign");
        }

        return {text, is_minus, IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query result;

        for (const string &word : SplitIntoWords(text))
        {
            QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    result.minus_words.insert(query_word.data);
                }
                else
                {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <class T>
    vector<Document> FindAllDocuments(const Query &query, T func) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto &[document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                if (func(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto &[document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto &[document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};