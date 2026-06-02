#pragma once

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <set>

#include "ReportServerInterface.h"
#include "ast/Ast.hpp"

using namespace ast;

namespace utils {
    void CreateUI(const ast::Node&                    node,
                  rapidjson::Value&                   response,
                  rapidjson::Document::AllocatorType& allocator);

    std::string FormatTimestampToString(const time_t&      timestamp,
                                        const std::string& format = "%Y.%m.%d %H:%M:%S");

    double TruncateDouble(const double& value, const int& digits);

    std::string GetGroupCurrencyByName(const std::vector<ReportGroupRecord>& group_vector,
                                       const std::string&                    group_name);

    std::string Trim(const std::string& str);

    std::set<std::string> SplitToSet(const std::string& str);
} // namespace utils