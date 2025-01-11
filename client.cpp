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
    struct nl_sock *sock;
    int family_id, ret;

    // Создание сокета
    sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate Netlink socket!" << std::endl;
        return -1;
    }

    // Подключение к Netlink
    if (genl_connect(sock)) {
        std::cerr << "Failed to connect to Netlink!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }

    // Получение ID семейства
    family_id = genl_ctrl_resolve(sock, FAMILY_NAME);
    if (family_id < 0) {
        std::cerr << "Unable to resolve family!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }

    // Создание JSON
    json request_json;
    request_json["action"] = "add";
    request_json["arg1"] = 4;
    request_json["arg"] = 5;

    // Лог исходящего запроса
    std::cout << "Sending request: " << request_json.dump() << std::endl;

    // Отправка данных через Netlink
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "Failed to allocate message!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, COMMAND_PROCESS, 1);
    nla_put_string(msg, 1, request_json.dump().c_str());

    ret = nl_send_auto(sock, msg);
    if (ret < 0) {
        std::cerr << "Failed to send message." << std::endl;
    }

    nlmsg_free(msg);
    nl_socket_free(sock);
    return 0;
}