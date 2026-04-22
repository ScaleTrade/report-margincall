#include "PluginInterface.h"

using namespace ast;

extern "C" void AboutReport(rapidjson::Value&                   request,
                            rapidjson::Value&                   response,
                            rapidjson::Document::AllocatorType& allocator,
                            ReportServerInterface*              server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Margin Call report", allocator), allocator);
    response.AddMember(
        "description",
        Value().SetString(
            "Lists accounts currently under margin call or stop out. "
            "Includes financial details such as balance, equity, margin, and full account details.",
            allocator),
        allocator);
    response.AddMember("type", static_cast<int>(ReportType::Group), allocator);
    response.AddMember("key", Value().SetString("MARGIN_CALL_REPORT", allocator), allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value&                   request,
                             rapidjson::Value&                   response,
                             rapidjson::Document::AllocatorType& allocator,
                             ReportServerInterface*              server) {
    std::string group_mask;

    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }

    std::unordered_map<std::string, Total>     totals_map;
    std::vector<ReportAccountRecord>           accounts_vector;
    std::vector<ReportGroupRecord>             groups_vector;
    std::unordered_map<int, ReportMarginLevel> margins_map;

    try {
        std::vector<ReportMarginLevel> margins_tmp_vector;

        server->GetAccountsByGroup(group_mask, &accounts_vector);
        server->GetAllGroups(&groups_vector);
        server->GetMarginLevelByGroup(group_mask, &margins_tmp_vector);

        for (const auto& margin_level : margins_tmp_vector) {
            margins_map[margin_level.login] = margin_level;
        }

    } catch (const std::exception& e) {
        std::cerr << "[MarginCallReportInterface]: " << e.what() << std::endl;
    }

    // Main table
    TableBuilder table_builder("MarginCallReportTable");

    // Main table props
    table_builder.SetIdColumn("login");
    table_builder.SetOrderBy("login", "DESC");
    table_builder.EnableAutoSave(false);
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    // Filters
    FilterConfig search_filter;
    search_filter.type = FilterType::Search;

    // Columns
    table_builder.AddColumn({"login", "LOGIN", 1, search_filter});
    table_builder.AddColumn({"name", "NAME", 2, search_filter});
    table_builder.AddColumn({"leverage", "LEVERAGE", 3, search_filter});
    table_builder.AddColumn({"balance", "BALANCE", 4, search_filter});
    table_builder.AddColumn({"credit", "CREDIT", 5, search_filter});
    table_builder.AddColumn({"floating_pl", "Floating P/L", 6, search_filter});
    table_builder.AddColumn({"equity", "EQUITY", 7, search_filter});
    table_builder.AddColumn({"margin", "MARGIN", 8, search_filter});
    table_builder.AddColumn({"margin_free", "MARGIN_FREE", 9, search_filter});
    table_builder.AddColumn({"margin_level", "MARGIN_LEVEL", 10, search_filter});
    table_builder.AddColumn({"currency", "CURRENCY", 11, search_filter});

    for (const auto& account : accounts_vector) {
        std::vector<ReportTradeRecord> trades_vector;
        double                         floating_pl  = 0.0;
        ReportMarginLevel              margin_level = margins_map[account.login];

        if (margin_level.level_type == MARGINLEVEL_MARGINCALL ||
            margin_level.level_type == MARGINLEVEL_STOPOUT) {

            double      multiplier = 1;
            std::string currency   = utils::GetGroupCurrencyByName(groups_vector, account.group);

            // Conversion disabled
            // if (currency != "USD") {
            //     try {
            //         server->CalculateConvertRateByCurrency(
            //             currency, "USD", static_cast<int>(ReportTradeCommand::Sell),
            //             &multiplier);
            //     } catch (const std::exception& e) {
            //         std::cerr << "[MarginCallReportInterface]: " << e.what() << std::endl;
            //     }
            // }

            floating_pl = margin_level.equity - margin_level.balance;

            totals_map[currency].balance += margin_level.balance * multiplier;
            totals_map[currency].credit += margin_level.credit * multiplier;
            totals_map[currency].floating_pl += floating_pl * multiplier;
            totals_map[currency].equity += margin_level.equity * multiplier;
            totals_map[currency].margin += margin_level.margin * multiplier;
            totals_map[currency].margin_free += margin_level.margin_free * multiplier;

            table_builder.AddRow({utils::TruncateDouble(account.login, 0),
                                  account.name,
                                  utils::TruncateDouble(margin_level.leverage, 0),
                                  utils::TruncateDouble(margin_level.balance * multiplier, 2),
                                  utils::TruncateDouble(margin_level.credit * multiplier, 2),
                                  utils::TruncateDouble(floating_pl * multiplier, 2),
                                  utils::TruncateDouble(margin_level.equity * multiplier, 2),
                                  utils::TruncateDouble(margin_level.margin * multiplier, 2),
                                  utils::TruncateDouble(margin_level.margin_free * multiplier, 2),
                                  utils::TruncateDouble(margin_level.margin_level, 2),
                                  currency});
        }
    }

    // Total row
    JSONArray totals_array;
    for (const auto& [currency, total] : totals_map) {
        totals_array.emplace_back(
            JSONObject{{"balance", utils::TruncateDouble(total.balance, 2)},
                       {"credit", utils::TruncateDouble(total.credit, 2)},
                       {"equity", utils::TruncateDouble(total.equity, 2)},
                       {"floating_pl", utils::TruncateDouble(total.floating_pl, 2)},
                       {"margin", utils::TruncateDouble(total.margin, 2)},
                       {"margin_free", utils::TruncateDouble(total.margin_free, 2)},
                       {"currency", currency}});
    }

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node       table_node  = Table({}, table_props);

    const Node report = Column({h1({text("Margin Call Report")}), table_node});

    utils::CreateUI(report, response, allocator);
}