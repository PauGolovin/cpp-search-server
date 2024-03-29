#pragma once
#include <string>
#include <iostream>
#include <vector>

struct Document {
    Document();

    Document(int id_, double relevance_, int rating_);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED, REMOVED, };