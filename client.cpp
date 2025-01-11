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

int main() {
    std::cout << "[Client] Starting client..." << std::endl;

    struct nl_sock *sock;
    int family_id, ret;

    // Создание сокета
    std::cout << "[Client] Allocating Netlink socket..." << std::endl;
    sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "[Client] Failed to allocate Netlink socket!" << std::endl;
        return -1;
    }
    std::cout << "[Client] Netlink socket successfully allocated." << std::endl;

    // Подключение к Netlink
    std::cout << "[Client] Connecting to Generic Netlink..." << std::endl;
    if (genl_connect(sock)) {
        std::cerr << "[Client] Failed to connect to Netlink!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }
    std::cout << "[Client] Successfully connected to Generic Netlink." << std::endl;

    // Получение ID семейства
    std::cout << "[Client] Resolving Generic Netlink family ID..." << std::endl;
    family_id = genl_ctrl_resolve(sock, FAMILY_NAME);
    if (family_id < 0) {
        std::cerr << "[Client] Unable to resolve family!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }
    std::cout << "[Client] Resolved family ID: " << family_id << std::endl;

    // Создание JSON
    json request_json;
    request_json["action"] = "add";
    request_json["arg1"] = 4;
    request_json["arg"] = 5;

    // Лог перед отправкой запроса
    std::cout << "[Client] JSON Request: " << request_json.dump() << std::endl;

    // Отправка данных через Netlink
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "[Client] Failed to allocate message!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }
    std::cout << "[Client] Preparing the Netlink message..." << std::endl;

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, COMMAND_PROCESS, 1);
    std::cout << "[Client] Attaching JSON data to Netlink message..." << std::endl;
    nla_put_string(msg, 1, request_json.dump().c_str());

    std::cout << "[Client] Sending Netlink message..." << std::endl;
    ret = nl_send_auto(sock, msg);
    if (ret < 0) {
        std::cerr << "[Client] Failed to send message!" << std::endl;
    } else {
        std::cout << "[Client] Message sent successfully." << std::endl;
    }

    // Освобождение ресурсов
    std::cout << "[Client] Cleaning up resources..." << std::endl;
    nlmsg_free(msg);
    nl_socket_free(sock);

    std::cout << "[Client] Client finished." << std::endl;
    return 0;
}