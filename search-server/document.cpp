//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎
#include "document.h"

Document::Document() = default;

Document::Document(int id_, double relevance_, int rating_)
    : id(id_)
    , relevance(relevance_)
    , rating(rating_) {
}