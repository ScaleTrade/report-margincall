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

    server->GetAccountsByGroup(group_mask, &accounts_vector);
    server->GetAllGroups(&groups_vector);

    // Фнкция для поиска валюты аккаунта по его группе
    auto get_group_currency = [&](const std::string& group_name) -> std::string {
        for (const auto& group : groups_vector) {
            if (group.group == group_name) {
                return group.currency;
            }
        }
        return "N/A"; // группа не найдена - валюта не определена
    };

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
            th({ text("Currency") }),
        }));

        // Формирование строк
        for (const auto& account :accounts_vector) {
            // Открытые сделки аккаунта
            std::vector<TradeRecord> trades_vector;

            if (server->GetOpenTradesByLogin(account.login, &trades_vector) == RET_OK) {
                double floating_pl = 0.0;
                MarginLevel margin_level_struct;

                server->GetAccountBalanceByLogin(account.login, &margin_level_struct);

                floating_pl = margin_level_struct.equity - margin_level_struct.balance;
                std::string currency = get_group_currency(account.group);

                auto& total = totals_map[currency];

                // Заполняем валюту (один раз)
                total.currency = currency;

                // Калькуляция
                total.balance     += margin_level_struct.balance;
                total.credit      += margin_level_struct.credit;
                total.floating_pl += floating_pl;
                total.equity      += margin_level_struct.equity;
                total.margin      += margin_level_struct.margin;
                total.margin_free += margin_level_struct.margin_free;

                std::cout << "=================" << std::endl;
                std::cout << "Login: " << account.login << std::endl;
                std::cout << "Name: " << account.name << std::endl;
                std::cout << "Leverage: " << margin_level_struct.leverage << std::endl;
                std::cout << "Balance: " << margin_level_struct.balance << std::endl;
                std::cout << "Credit: " << margin_level_struct.credit << std::endl;
                std::cout << "Floating P/L: " << floating_pl << std::endl;
                std::cout << "Equity: " << margin_level_struct.equity << std::endl;
                std::cout << "Margin: " << margin_level_struct.margin << std::endl;
                std::cout << "Free Margin: " << margin_level_struct.margin_free << std::endl;
                std::cout << "Margin Level: " << margin_level_struct.margin_level << std::endl;
                std::cout << "Currency: " << currency << std::endl;
                std::cout << "=================" << std::endl;

                table_rows.push_back(tr({
                    td({ text(std::to_string(account.login)) }),
                    td({ text(account.name) }),
                    td({ text(std::to_string(margin_level_struct.leverage)) }),
                    td({ text(std::to_string(margin_level_struct.balance)) }),
                    td({ text(std::to_string(margin_level_struct.credit)) }),
                    td({ text(std::to_string(floating_pl)) }),
                    td({ text(std::to_string(margin_level_struct.equity)) }),
                    td({ text(std::to_string(margin_level_struct.margin)) }),
                    td({ text(std::to_string(margin_level_struct.margin_free)) }),
                    td({ text(std::to_string(margin_level_struct.margin_level)) }),
                    td({ text(currency) }),
                }));
            }
        }

        // Формирование строк Total
        for (const auto& pair : totals_map) {
            const Total& total = pair.second;

            table_rows.push_back(tr({
                td({ text("TOTAL") }),
                td({ text("") }),
                td({ text("") }),
                td({ text(std::to_string(total.balance)) }),
                td({ text(std::to_string(total.credit)) }),
                td({ text(std::to_string(total.floating_pl)) }),
                td({ text(std::to_string(total.equity)) }),
                td({ text(std::to_string(total.margin)) }),
                td({ text(std::to_string(total.margin_free)) }),
                td({ text("") }),
                td({ text("") }),
                td({ text(total.currency) }),
            }));
        }

        std::cout << "Total: " << std::endl;
        for (const auto& pair : totals_map) {
            const Total& total = pair.second;

            std::cout << "Currency: "     << total.currency      << std::endl;
            std::cout << "  Balance: "    << total.balance       << std::endl;
            std::cout << "  Credit: "     << total.credit        << std::endl;
            std::cout << "  Floating P/L: " << total.floating_pl << std::endl;
            std::cout << "  Equity: "     << total.equity        << std::endl;
            std::cout << "  Margin: "     << total.margin        << std::endl;
            std::cout << "  Free Margin: "<< total.margin_free   << std::endl;
            std::cout << "=================" << std::endl;
        }

        return table(table_rows, props({{"className", "data-table"}}));
    };

    const Node report = div({
        h1({ text("Margin Call Report") }),
        make_table(accounts_vector)
    }, props({{"className", "report"}}));

    to_json(report, response, allocator);
}