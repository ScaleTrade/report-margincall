#pragma once

#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include "Structures.hpp"
#include "ast/Ast.hpp"
#include "Structures.hpp"

namespace utils {
    void CreateUI(const ast::Node& node,
              rapidjson::Value& response,
              rapidjson::Document::AllocatorType& allocator);

    std::string FormatTimestampToString(const time_t& timestamp);

    double TruncateDouble(const double& value, const int& digits);

    std::string GetGroupCurrencyByName(const std::vector<GroupRecord>& group_vector, const std::string& group_name);
}