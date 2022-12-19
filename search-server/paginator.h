#pragma once

template <typename Iterator>
class Paginator {
    // тело класса
public:
    Paginator(Iterator ItBegin, Iterator ItEnd, int size) {
        Iterator It = ItBegin;
        while (It != ItEnd) {
            std::vector<typename Iterator::value_type> page;
            int i = 0;
            while (i < size && It != ItEnd) {
                page.push_back(*It);
                ++It;
                ++i;
            }
            pages_.push_back(page);
        }
    }

    std::vector<std::vector<typename Iterator::value_type>> GetPages() {
        return pages_;
    }
private:
    std::vector<std::vector<typename Iterator::value_type>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    Paginator pages(begin(c), end(c), page_size);
    return pages.GetPages();
}