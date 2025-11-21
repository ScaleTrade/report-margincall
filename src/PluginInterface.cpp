#include "PluginInterface.hpp"

using namespace ast;

extern "C" void AboutReport(rapidjson::Value& request,
                            rapidjson::Value& response,
                            rapidjson::Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Margin call report", allocator), allocator);
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
    // Структура накопления итогов
    struct Total {
        std::string currency;
        double balance = 0.0;
        double credit = 0.0;
        double floating_pl = 0.0;
        double equity = 0.0;
        double margin = 0.0;
        double margin_free = 0.0;
    };

    std::unordered_map<std::string, Total> totals_map;

    std::string group_mask;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }

    std::vector<AccountRecord> accounts_vector;
    std::vector<GroupRecord> groups_vector;

    try {
        server->GetAccountsByGroup(group_mask, &accounts_vector);
        server->GetAllGroups(&groups_vector);
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

    // Лямбда подготавливающая значения double для вставки в AST (округление до 2х знаков)
    auto format_for_AST = [](double value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };

    // Таблица
    auto make_table = [&](const std::vector<AccountRecord>& accounts) -> Node {
        std::vector<Node> table_rows;

        // Заголовки
        table_rows.push_back(thead({
            tr({
                th({div({text("Login")})}),
                th({div({text("Name")})}),
                th({div({text("Description")})}),
                th({div({text("Balance")})}),
                th({div({text("Credit")})}),
                th({div({text("Floating P/L")})}),
                th({div({text("Equity")})}),
                th({div({text("Margin")})}),
                th({div({text("Free Margin")})}),
                th({div({text("Margin Level")})}),
                th({div({text("Currency")})}),
            })
        }));

        // Формирование строк
        for (const auto& account : accounts) {
            std::vector<TradeRecord> trades_vector; // открытые сделки аккаунта
            double floating_pl = 0.0;
            MarginLevel margin_level;

            server->GetAccountBalanceByLogin(account.login, &margin_level);

            if (margin_level.level_type == MARGINLEVEL_MARGINCALL || margin_level.level_type == MARGINLEVEL_STOPOUT) {
                floating_pl = margin_level.equity - margin_level.balance;
                std::string currency = get_group_currency(account.group);

                auto& total = totals_map[currency];

                total.currency = currency;
                total.balance += margin_level.balance;
                total.credit += margin_level.credit;
                total.floating_pl += floating_pl;
                total.equity += margin_level.equity;
                total.margin += margin_level.margin;
                total.margin_free += margin_level.margin_free;

                table_rows.push_back(tr({
                    td({div({text(std::to_string(account.login))})}),
                    td({div({text(account.name)})}),
                    td({div({text(format_for_AST(margin_level.leverage))})}),
                    td({div({text(format_for_AST(margin_level.balance))})}),
                    td({div({text(format_for_AST(margin_level.credit))})}),
                    td({div({text(format_for_AST(floating_pl))})}),
                    td({div({text(format_for_AST(margin_level.equity))})}),
                    td({div({text(format_for_AST(margin_level.margin))})}),
                    td({div({text(format_for_AST(margin_level.margin_free))})}),
                    td({div({text(format_for_AST(margin_level.margin_level))})}),
                    td({div({text(currency)})}),
                }));
            }
        }

        // Заголовок Total
        table_rows.push_back(thead({
            tr({
                th({div({text("TOTAL:")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
                th({div({text("")})}),
            })
        }));

        // Формирование строк Total
        for (const auto& pair : totals_map) {
            const Total& total = pair.second;

            table_rows.push_back(tr({
                td({ text("") }),
                td({ text("") }),
                td({ text("") }),
                td({ text(format_for_AST(total.balance)) }),
                td({ text(format_for_AST(total.credit)) }),
                td({ text(format_for_AST(total.floating_pl)) }),
                td({ text(format_for_AST(total.equity)) }),
                td({ text(format_for_AST(total.margin)) }),
                td({ text(format_for_AST(total.margin_free)) }),
                td({ text("") }),
                td({ text(total.currency) }),
            }));
        }

        return table(table_rows, props({{"className", "table"}}));
    };


    const Node report = div({
        h1({ text("Margin Call Report") }),
        make_table(accounts_vector),
    });

    utils::CreateUI(report, response, allocator);
}