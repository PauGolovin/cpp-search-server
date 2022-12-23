#pragma once
#include <algorithm>
#include <numeric>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "document.h"
#include "string_processing.h"
#include "search_server.h"

// Удаление дубликатов
void RemoveDuplicates(SearchServer& search_server);