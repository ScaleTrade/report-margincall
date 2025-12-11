#include "PluginInterface.hpp"

using namespace ast;

extern "C" void AboutReport(rapidjson::Value& request,
                            rapidjson::Value& response,
                            rapidjson::Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Margin Call report", allocator), allocator);
    response.AddMember(
        "description",
        Value().SetString("Lists accounts currently under margin call or stop out. "
                          "Includes financial details such as balance, equity, margin, and full account details.",
                          allocator), allocator);
    response.AddMember("type", REPORT_GROUP_TYPE, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value& request,
                             rapidjson::Value& response,
                             rapidjson::Document::AllocatorType& allocator,
                             CServerInterface* server) {
    std::string group_mask;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }

    std::unordered_map<std::string, Total> totals_map;
    std::vector<AccountRecord> accounts_vector;
    std::vector<GroupRecord> groups_vector;
    std::unordered_map<int, MarginLevel> margins_map;

    try {
        std::vector<MarginLevel> margins_tmp_vector;

        server->GetAccountsByGroup(group_mask, &accounts_vector);
        server->GetAllGroups(&groups_vector);
        server->GetMarginLevelByGroup(group_mask, &margins_tmp_vector);

        for (const auto& margin_level : margins_tmp_vector) {
            margins_map[margin_level.login] = margin_level;
        }

    } catch (const std::exception& e) {
        std::cerr << "[MarginCallReportInterface]: " << e.what() << std::endl;
    }

    // Лямбда для поиска валюты аккаунта по группе
    auto get_group_currency = [&](const std::string& group_name) -> std::string {
        for (const auto& group : groups_vector) {
            if (group.group == group_name) {
                return group.currency;
            }
        }
        return "N/A"; // группа не найдена - валюта не определена
    };

    // Лямбда подготавливающая значения double для вставки в AST (округление до 2-х знаков)
    auto format_double_for_AST = [](double value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };

    TableBuilder table_builder("MarginCallReportTable");

    table_builder.SetIdColumn("login");
    table_builder.SetOrderBy("login", "DESC");
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    table_builder.AddColumn({"login", "LOGIN"});
    table_builder.AddColumn({"name", "NAME"});
    table_builder.AddColumn({"leverage", "LEVERAGE"});
    table_builder.AddColumn({"balance", "BALANCE"});
    table_builder.AddColumn({"credit", "CREDIT"});
    table_builder.AddColumn({"floating_pl", "Floating P/L"});
    table_builder.AddColumn({"equity", "EQUITY"});
    table_builder.AddColumn({"margin", "MARGIN"});
    table_builder.AddColumn({"margin_free", "MARGIN_FREE"});
    table_builder.AddColumn({"margin_level", "MARGIN_LEVEL"});
    table_builder.AddColumn({"currency", "CURRENCY"});

    for (const auto& account : accounts_vector) {
        std::vector<TradeRecord> trades_vector;
        double floating_pl = 0.0;
        MarginLevel margin_level = margins_map[account.login];

        if (margin_level.level_type == MARGINLEVEL_MARGINCALL ||
            margin_level.level_type == MARGINLEVEL_STOPOUT) {

            double multiplier = 1;
            std::string currency = get_group_currency(account.group);

            if (currency != "USD") {
                try {
                    server->CalculateConvertRateByCurrency(currency, "USD", OP_SELL, &multiplier);
                } catch (const std::exception& e) {
                    std::cerr << "[MarginCallReportInterface]: " << e.what() << std::endl;
                }
            }

            floating_pl = margin_level.equity - margin_level.balance;

            totals_map["USD"].balance += margin_level.balance * multiplier;
            totals_map["USD"].credit += margin_level.credit * multiplier;
            totals_map["USD"].floating_pl += floating_pl * multiplier;
            totals_map["USD"].equity += margin_level.equity * multiplier;
            totals_map["USD"].margin += margin_level.margin * multiplier;
            totals_map["USD"].margin_free += margin_level.margin_free * multiplier;

            table_builder.AddRow({
                {"login", std::to_string(account.login)},
                {"name", account.name},
                {"leverage", format_double_for_AST(margin_level.leverage)},
                {"balance", format_double_for_AST(margin_level.balance * multiplier)},
                {"credit", format_double_for_AST(margin_level.credit * multiplier)},
                {"floating_pl", format_double_for_AST(floating_pl * multiplier)},
                {"equity", format_double_for_AST(margin_level.equity * multiplier)},
                {"margin", format_double_for_AST(margin_level.margin * multiplier)},
                {"margin_free", format_double_for_AST(margin_level.margin_free * multiplier)},
                {"margin_level", format_double_for_AST(margin_level.margin_level)},
                {"currency", "USD"}
            });
        }
    }

    // Total row
    JSONArray totals_array;
    totals_array.emplace_back(JSONObject{
        {"balance", totals_map["USD"].balance},
        {"credit", totals_map["USD"].credit},
        {"equity", totals_map["USD"].equity},
        {"floating_pl", totals_map["USD"].floating_pl},
        {"margin", totals_map["USD"].margin},
        {"margin_free", totals_map["USD"].margin_free},
    });

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node table_node = Table({}, table_props);

    const Node report = div({
        h1({ text("Margin Call Report") }),
        table_node
    });

    utils::CreateUI(report, response, allocator);
}