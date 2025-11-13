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
    std::string group_mask;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }

    std::vector<AccountRecord> accounts_vector;

    server->GetAccountsByGroup(group_mask, &accounts_vector);

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
            // th({ text("Add. Margin") }),
            // th({ text("Currency") }),
        }));

        // Формирование строк
        for (const auto& account :accounts_vector) {
            double pl = 0.0;
            double margin = 0.0;

            // Открытые сделки аккаунта
            std::vector<TradeRecord> trades_vector;

            if (server->GetOpenTradesByLogin(account.login, &trades_vector) == RET_OK) {
                for (const auto& trade : trades_vector) {
                    double trade_profit = 0.0;
                    double trade_swap = 0.0;
                    double trade_commission = 0.0;
                    double trade_margin = 0.0;

                    server->CalculateProfit(trade, &trade_profit);
                    server->CalculateSwap(trade, &trade_swap);
                    server->CalculateCommission(trade, &trade_commission);
                    server->CalculateMargin(trade, &trade_margin);

                    pl += trade_profit + trade_swap + trade_commission;
                    margin += trade_margin;
                }
            }

            const double equity = account.balance + account.credit + margin;
            const double free_margin = equity - margin;
            const double margin_level = margin > 0.0 ? (equity / margin) * 100.0 : 0.0;

            std::cout << "Login: " << account.login << std::endl;
            std::cout << "Name: " << account.name << std::endl;
            std::cout << "Leverage: " << account.leverage << std::endl;
            std::cout << "Balance: " << account.balance << std::endl;
            std::cout << "Credit: " << account.credit << std::endl;
            std::cout << "Floating P/L: " << pl << std::endl;
            std::cout << "Equity: " << equity << std::endl;
            std::cout << "Margin: " << margin << std::endl;
            std::cout << "Free Margin: " << free_margin << std::endl;
            std::cout << "Margin Level: " << margin_level << std::endl;

            table_rows.push_back(tr({
                td({ text(std::to_string(account.login)) }),
                td({ text(account.name) }),
                td({ text(std::to_string(account.leverage)) }),
                td({ text(std::to_string(account.balance)) }),
                td({ text(std::to_string(account.credit)) }),
                td({ text(std::to_string(pl)) }),
                td({ text(std::to_string(equity)) }),
                td({ text(std::to_string(margin)) }),
                td({ text(std::to_string(free_margin)) }),
                td({ text(std::to_string(margin_level)) }),
            }));
        }



        return table(table_rows, props({{"className", "data-table"}}));
    };

    const Node report = div({
        h1({ text("Margin Call Report") }),
        make_table(accounts_vector)
    }, props({{"className", "report"}}));

    to_json(report, response, allocator);
}