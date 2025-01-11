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
    struct genlmsghdr *genl_hdr = static_cast<struct genlmsghdr *>(nlmsg_data(nlhdr));

    // Проверка команды
    if (genl_hdr->cmd != COMMAND_PROCESS) {
        std::cerr << "Unknown command received!" << std::endl;
        return;
    }

    // Разбор JSON-строки
    struct nlattr *attrs[5];
    genlmsg_parse(nlhdr, 0, attrs, 5, nullptr);
    const char *json_string = static_cast<const char *>(nla_data(attrs[1]));
    try {
        json parsed_json = json::parse(json_string);
        std::string action = parsed_json["action"];
        int arg1 = parsed_json["arg1"];
        int arg2 = parsed_json["arg"];
        int result = 0;

        // Выполнение действия
        if (action == "add") {
            result = arg1 + arg2;
        } else if (action == "sub") {
            result = arg1 - arg2;
        } else if (action == "mul") {
            result = arg1 * arg2;
        } else {
            std::cerr << "Unknown action: " << action << std::endl;
            return;
        }

        // Формирование ответа JSON
        json response_json;
        response_json["result"] = result;

        // Лог
        std::cout << "Response: " << response_json.dump() << std::endl;

        // Здесь отправка ответа клиенту через Netlink (не реализовано в этой версии)
    } catch (const json::exception &e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Starting server..." << std::endl;

    // Здесь будет регистрация семейства и обработчиков через Generic Netlink

    std::cout << "Server finished." << std::endl;
    return 0;
}