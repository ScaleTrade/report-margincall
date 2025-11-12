#include "MarginCallPluginInterface.hpp"

using namespace ast;

extern "C" void AboutReport(Value& request,
                            Value& response,
                            Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Margin call report", allocator), allocator);
    response.AddMember("description", Value().SetString("Report with table", allocator), allocator);
    response.AddMember("type", REPORT_NONE_TYPE, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(Value& request,
                             Value& response,
                             Document::AllocatorType& allocator,
                             CServerInterface* server) {

    std::cout << "Creating report..." << std::endl;

    const int api_version = server->GetApiVersion();
    const std::string test_string = "API VERSION: " + std::to_string(api_version);

    struct DayData {
        std::string day;
        double commission;
        double profit;
    };

    std::vector<DayData> data = {
        {"Mon", 120.5, 80.2},
        {"Tue", 150.3, 95.6},
        {"Wed", 130.1, 88.0},
        {"Thu", 170.2, 110.3},
        {"Fri", 160.8, 102.4}
    };

    // ---------- Table ----------
    auto makeTable = [&](const std::vector<DayData>& rows) -> Node {
        std::vector<Node> tableRows;

        // Заголовок
        tableRows.push_back(tr({
            th({ text("Day") }),
            th({ text("Commission") }),
            th({ text("P/L") })
        }));

        // Динамические строки (через лямбду)
        for (const auto& row : rows) {
            tableRows.push_back(tr({
                td({ text(row.day) }),
                td({ text(std::to_string(row.commission)) }),
                td({ text(std::to_string(row.profit)) })
            }));
        }

        return table(tableRows, props({{"className", "data-table"}}));
    };

    Node report = div({
        h1({ text(test_string) }),
        makeTable(data)
    }, props({{"className", "report"}}));


    to_json(report, response, allocator);

    std::cout << "Creating report..." << std::endl;
}