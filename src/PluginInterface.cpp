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

    TableBuilder table_builder("MarginCallReportTable");

    table_builder.SetIdColumn("login");
    table_builder.SetOrderBy("login", "DESC");
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    table_builder.AddColumn({"login", "LOGIN", 1});
    table_builder.AddColumn({"name", "NAME", 2});
    table_builder.AddColumn({"leverage", "LEVERAGE", 3});
    table_builder.AddColumn({"balance", "BALANCE", 4});
    table_builder.AddColumn({"credit", "CREDIT", 5});
    table_builder.AddColumn({"floating_pl", "Floating P/L", 6});
    table_builder.AddColumn({"equity", "EQUITY", 7});
    table_builder.AddColumn({"margin", "MARGIN", 8});
    table_builder.AddColumn({"margin_free", "MARGIN_FREE", 9});
    table_builder.AddColumn({"margin_level", "MARGIN_LEVEL", 10});
    table_builder.AddColumn({"currency", "CURRENCY", 11});

    for (const auto& account : accounts_vector) {
        std::vector<TradeRecord> trades_vector;
        double floating_pl = 0.0;
        MarginLevel margin_level = margins_map[account.login];

        if (margin_level.level_type == MARGINLEVEL_MARGINCALL ||
            margin_level.level_type == MARGINLEVEL_STOPOUT) {

            double multiplier = 1;
            std::string currency = utils::GetGroupCurrencyByName(groups_vector, account.group);

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
                {"login", utils::TruncateDouble(account.login, 0)},
                {"name", account.name},
                {"leverage", utils::TruncateDouble(margin_level.leverage, 0)},
                {"balance", utils::TruncateDouble(margin_level.balance * multiplier, 2)},
                {"credit", utils::TruncateDouble(margin_level.credit * multiplier, 2)},
                {"floating_pl", utils::TruncateDouble(floating_pl * multiplier, 2)},
                {"equity", utils::TruncateDouble(margin_level.equity * multiplier, 2)},
                {"margin", utils::TruncateDouble(margin_level.margin * multiplier, 2)},
                {"margin_free", utils::TruncateDouble(margin_level.margin_free * multiplier, 2)},
                {"margin_level", utils::TruncateDouble(margin_level.margin_level, 2)},
                {"currency", "USD"}
            });
        }
    }

    // Total row
    JSONArray totals_array;
    totals_array.emplace_back(JSONObject{
        {"balance", utils::TruncateDouble(totals_map["USD"].balance, 2)},
        {"credit", utils::TruncateDouble(totals_map["USD"].credit, 2)},
        {"equity", utils::TruncateDouble(totals_map["USD"].equity, 2)},
        {"floating_pl", utils::TruncateDouble(totals_map["USD"].floating_pl, 2)},
        {"margin", utils::TruncateDouble(totals_map["USD"].margin, 2)},
        {"margin_free", utils::TruncateDouble(totals_map["USD"].margin_free, 2)},
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