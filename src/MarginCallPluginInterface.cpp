#include "MarginCallPluginInterface.hpp"

using namespace ast;

extern "C" void AboutReport(rapidjson::Value& request,
                            rapidjson::Value& response,
                            rapidjson::Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Margin call report", allocator), allocator);
    response.AddMember("description", Value().SetString("Report with table", allocator), allocator);
    response.AddMember("type", REPORT_GROUP_TYPE, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value& request,
                             rapidjson::Value& response,
                             rapidjson::Document::AllocatorType& allocator,
                             CServerInterface* server) {
    std::cout << "Creating margin call report.." << std::endl;

    std::vector<TradeRecord> closed_trades;
    int result = 2;

    LogJSON("request", request);

    // try {
    //     result = server->GetCloseTradesByLogin(2, &closed_trades);
    // } catch (const std::exception& e) {
    //     std::cerr << e.what() << std::endl;
    // }

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

    // ---------- Таблица ----------
    auto makeTable = [&](const std::vector<DayData>& rows) -> Node {
        std::vector<Node> tableRows;

        // Заголовок
        tableRows.push_back(tr({
            th({ text("Login") }),
            th({ text("Name") }),
            th({ text("Leverage") }),
            th({ text("Balance") }),
            th({ text("Credit") }),
            th({ text("Floating P/L") }),
            th({ text("Equity") }),
            th({ text("Margin") }),
            th({ text("Free Margin") }),
            th({ text("Margin Limits") }),
            th({ text("Margin Level") }),
            th({ text("Add. Margin") }),
            th({ text("Currency") }),
        }));

        // Динамическое формирование строк (через лямбду)
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
        h1({ text("Margin Call Report") }),
        makeTable(data)
    }, props({{"className", "report"}}));


    to_json(report, response, allocator);
}

void LogJSON(const std::string& name, const Value& value) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    value.Accept(writer); // сериализуем JSON в строку
    std::cout << "[LogJSON]: " << name << ": " << buffer.GetString() << std::endl;
}