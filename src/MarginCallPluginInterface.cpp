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
    LogJSON("request: ", request);

    std::string group_mask;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }

    std::vector<GroupRecord> groups_vector;
    std::vector<AccountRecord> accounts_vector;

    if (group_mask == "*") {
        const int groups_result = server->GetAllGroups(&groups_vector);

        for (const GroupRecord& group : groups_vector) {
            server->GetAccountsByGroup(group.group, &accounts_vector);
        }
    } else {
        server->GetAccountsByGroup(group_mask, &accounts_vector);
    }

    // Таблица
    auto make_table = [&](const std::vector<AccountRecord>& accounts) -> Node {
        std::vector<Node> table_rows;

        // Заголовки
        table_rows.push_back(tr({
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

        return table(table_rows, props({{"className", "data-table"}}));
    };

    std::cout << "Groups vector size: " << groups_vector.size()<< std::endl;
    std::cout << "Accounts vector size: " << accounts_vector.size()<< std::endl;

    const Node report = div({
        h1({ text("Margin Call Report") }),
        make_table(accounts_vector)
    }, props({{"className", "report"}}));

    to_json(report, response, allocator);
}

void LogJSON(const std::string& name, const Value& value) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    value.Accept(writer);
    std::cout << "[LogJSON]: " << name << ": " << buffer.GetString() << std::endl;
}