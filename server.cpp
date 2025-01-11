#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/netlink.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

#define FAMILY_NAME "calc_family"
#define COMMAND_SERVER 2

using json = nlohmann::json;

enum {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};

// Функция для обработки сообщений
static int receive_message(struct nl_msg *msg, void *arg) {
    // Получение полезной нагрузки
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct genlmsghdr *genl_hdr = (genlmsghdr *)nlmsg_data(nlh);
    struct nlattr *attrs[ATTR_MAX + 1];

    genlmsg_parse(nlh, 0, attrs, ATTR_MAX, nullptr);

    if (attrs[ATTR_MSG]) {
        const char *data = nla_get_string(attrs[ATTR_MSG]);
        std::cout << "Received message from kernel: " << data << std::endl;
    } else {
        std::cerr << "Message has no payload!" << std::endl;
    }

    return NL_OK;
}

int main() {
    std::cout << "Starting server..." << std::endl;

    struct nl_sock *sock;
    int family_id, ret;

    // Создание сокета
    sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate Netlink socket!" << std::endl;
        return -1;
    }

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

    // Регистрация обработчика сообщений
    nl_socket_modify_cb(sock, NL_CB_MSG_IN, NL_CB_CUSTOM, receive_message, nullptr);

    // Создание JSON-запроса
    json request_json;
    request_json["message"] = "Hello";

    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "Failed to allocate message!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }

    if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, COMMAND_SERVER, 1)) {
        std::cerr << "Failed to create Netlink message header!" << std::endl;
        nlmsg_free(msg);
        nl_socket_free(sock);
        return -1;
    }

    if (nla_put_string(msg, ATTR_MSG, request_json.dump().c_str())) {
        std::cerr << "Failed to attach JSON payload!" << std::endl;
        nlmsg_free(msg);
        nl_socket_free(sock);
        return -1;
    }

    ret = nl_send_auto(sock, msg);
    if (ret < 0) {
        std::cerr << "Failed to send message!" << std::endl;
    } else {
        std::cout << "Message sent successfully." << std::endl;
    }

    nlmsg_free(msg);

    // Ожидание ответов (здесь можно ожидать в цикле)
    // std::cout << "Waiting for responses from kernel..." << std::endl;
    // nl_recvmsgs_default(sock);
    std::cout << "Waiting for responses from kernel..." << std::endl;
    while (true) {
        int ret = nl_recvmsgs_default(sock);
        if (ret < 0) {
            std::cerr << "Error receiving message: " << nl_geterror(ret) << std::endl;
            break;  // Выйти из цикла в случае ошибки, если необходимо
        }
    }

    nl_socket_free(sock);
    std::cout << "Client finished." << std::endl;
    return 0;
}