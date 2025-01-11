#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <cstring>
#include <cstdlib>

// Generic Netlink family name
#define FAMILY_NAME "calc_family"
#define COMMAND_PROCESS 1

using json = nlohmann::json; // Удобный псевдоним для библиотеки

void handle_request(struct nl_msg *msg, struct nlmsghdr *nlhdr) {
    std::cout << "[Server] Received a message...\n";

    struct genlmsghdr *genl_hdr = static_cast<struct genlmsghdr *>(nlmsg_data(nlhdr));

    // Проверка команды
    if (genl_hdr->cmd != COMMAND_PROCESS) {
        std::cerr << "[Server] Unknown command received!" << std::endl;
        return;
    }

    // Разбор JSON-строки
    struct nlattr *attrs[5];
    genlmsg_parse(nlhdr, 0, attrs, 5, nullptr);
    const char *json_string = static_cast<const char *>(nla_data(attrs[1]));

    try {
        // Лог входящего JSON запроса
        std::cout << "[Server] Parsing JSON: " << json_string << std::endl;

        json parsed_json = json::parse(json_string);
        std::string action = parsed_json["action"];
        int arg1 = parsed_json["arg1"];
        int arg2 = parsed_json["arg"];
        int result = 0;

        // Лог входящих параметров
        std::cout << "[Server] Action: " << action << ", arg1: " << arg1 << ", arg2: " << arg2 << std::endl;

        // Выполнение действия
        if (action == "add") {
            result = arg1 + arg2;
            std::cout << "[Server] Performing addition." << std::endl;
        } else if (action == "sub") {
            result = arg1 - arg2;
            std::cout << "[Server] Performing subtraction." << std::endl;
        } else if (action == "mul") {
            result = arg1 * arg2;
            std::cout << "[Server] Performing multiplication." << std::endl;
        } else {
            std::cerr << "[Server] Unknown action: " << action << std::endl;
            return;
        }

        // Формирование ответа JSON
        json response_json;
        response_json["result"] = result;

        // Лог результата до отправки
        std::cout << "[Server] Response JSON: " << response_json.dump() << std::endl;

        // Отправка ответа клиенту (нужно будет интегрировать send через Netlink)
    } catch (const json::exception &e) {
        std::cerr << "[Server] JSON parsing error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "[Server] Starting server..." << std::endl;

    // Здесь будет регистрация семейства и обработчиков через Generic Netlink
    std::cout << "[Server] Waiting for messages on Generic Netlink..." << std::endl;

    std::cout << "[Server] Server finished." << std::endl;
    return 0;
}