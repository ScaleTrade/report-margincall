#pragma once

#include <sstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <iomanip>
#include <unordered_map>
#include <iostream>

#include "rapidjson/document.h"
#include "ReportServerInterface.h"
#include "ast/Ast.hpp"
#include "sbxTableBuilder/SBXTableBuilder.hpp"
#include "structures/ReportStructures.hpp"
#include "structures/ReportType.h"
#include "utils/Utils.h"

using namespace ast;

extern "C" {
    int GetReportApiVersion();

    void AboutReport(rapidjson::Value& request,
                     rapidjson::Value& response,
                     rapidjson::Document::AllocatorType& allocator,
                     ReportServerInterface* server);

    void DestroyReport();

    void CreateReport(rapidjson::Value& request,
                     rapidjson::Value& response,
                     rapidjson::Document::AllocatorType& allocator,
                     ReportServerInterface* server);
}