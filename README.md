<h1><i> Search Server</i></h1>

## **Overview**

Search Server is a keyword search system that implements the following functionality:
* Ranking of documents by TF-IDF
* Ability to perform document search in multithreaded mode
* Processing of stop- and minus-words
* Deleting duplicate documents
* Paginated output of search results
_____
______
## **Description**

### **Principle of operation**

Creating an instance of the SearchServer class. The constructor is passed a string with stop words separated by spaces. A standard container can be passed sequentially (with subsequent access to the element with the possibility of using it in a for-range loop).

The AddDocument method adds documents to be searched. The method passes the document id, status, rating, and the document itself in string format.

The FindTopDocuments method returns vector documents according to the passed nutrients. The results are sorted by the TF-IDF statistic. Additional filtering of documents by ID, status and rating is possible. Implementation method in both single-threaded and multi-threaded versions.
____
### **Input Data**

Search Server is created using a constructor whose parameter is a set of stop words passed as string, string_view, or a string container:

```
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(std::string_view stop_words_text);
    explicit SearchServer(const std::string& stop_words_text);
```

Adding a document to the server is done with the **AddDocument** command. As parameters, you must pass the document id, status, rating container and the text of the document itself:

```
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
```
Deleting a document is performed using the **RemoveDocument** command, which can be run in multithreaded mode:
```
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
```

_____ 

### **Output Data**

The main functionality of interaction with the Search Server is in the **FindTopDocuments** and **MatchDocument** functions. They accept a query as a string_view and considers minus-words written in the format *"-minusword"*.

**FindTopDocuments** displays the most relevant documents for a query. Their number is set by the *MAX_RESULT_DOCUMENT_COUNT* value:
```
    const int MAX_RESULT_DOCUMENT_COUNT = 5;
```
**FindTopDocuments** accepts a query as a string_view. Also, a status or a predicate function for filtering documents by id, status, and rating can be passed as parameters. **FindTopDocuments** can be run in multithreaded mode:
```
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
```
**MatchDocument** accepts a query as a string_view and a document id, and returns a vector<string_view> of words from the document that match the query and the status of the document. **MatchDocument** can be run in multithreaded mode:
```
    using MatchDocument_Type = std::tuple<std::vector<std::string_view>, DocumentStatus>;

    MatchDocument_Type MatchDocument(std::string_view raw_query, int document_id) const;
    MatchDocument_Type MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    MatchDocument_Type MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;
```
Other functions:

**GetDocumentCount** function returns count of documents at Search Server.

**GetWordFrequencies** function accepts a document id and returns frequency for all words in the document in the format:
```
    std::map<std::string_view, double>
``` 
_____ 
### **Paginator**

For the Search Server, the **Paginator** class is implemented and defined in the file *paginator.h*.

This class is initialized with iterators of the beginning and end of the container to be paginated, as well as the number of results per page. **Paginator::GetPages()** method returns a container of containers with paginated results.

_____
______
## **System requirements**

C++ compiler supporting C++17 standard or later.
