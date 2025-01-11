#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>

#define FAMILY_NAME "calc_family"
#define COMMAND_SERVER 2

using json = nlohmann::json;

enum {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};

struct nl_sock *sock;
int family_id;

using json = nlohmann::json;

bool send_message(nl_sock *sock, int family_id, int command, const std::string &payload) {
    std::cout << "Sending message: " << payload << std::endl;
    nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "Failed to allocate message!" << std::endl;
        return false;
    }

    if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, command, 1)) {
        std::cerr << "Failed to create Netlink message header!" << std::endl;
        nlmsg_free(msg);
        return false;
    }

    if (nla_put_string(msg, ATTR_MSG, payload.c_str())) {
        std::cerr << "Failed to attach JSON payload!" << std::endl;
        nlmsg_free(msg);
        return false;
    }

    int ret = nl_send_auto(sock, msg);
    if (ret < 0) {
        std::cerr << "Failed to send message!" << std::endl;
        nlmsg_free(msg);
        return false;
    } else {
        nlmsghdr *nlh = nlmsg_hdr(msg);
        std::cout << "Message sent successfully with seq: " << nlh->nlmsg_seq << std::endl;
    }

    nlmsg_free(msg);
    return true;
}

nlohmann::json process_request(const std::string &request_json) {
    try {
        std::cout << "Processing request: " << request_json << std::endl;
        // Парсим JSON из строки
        json request = json::parse(request_json);

        // Проверяем наличие ключей "action", "arg1", "arg2"
        if (!request.contains("action") || !request.contains("arg1") || !request.contains("arg2")) {
            return json({{"error", "Invalid input. Missing fields 'action', 'arg1', or 'arg2'"}});
        }

        // Извлекаем действие и аргументы
        std::string action = request.at("action").get<std::string>();
        int arg1 = request.at("arg1").get<int>();
        int arg2 = request.at("arg2").get<int>();
        int result = 0;

        // Выполняем указанное действие
        if (action == "add") {
            result = arg1 + arg2;
        } else if (action == "sub") {
            result = arg1 - arg2;
        } else if (action == "mul") {
            result = arg1 * arg2;
        } else {
            return R"({ "error": "Invalid action. Supported actions are 'add', 'sub', 'mul'" })";
        }

        // Формируем и возвращаем результат
        json response;
        response["result"] = result;
        return response;
    } catch (const std::exception &e) {
        return R"({ "error": "Invalid JSON format or internal error" })";
    }
}

static int receive_message(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct genlmsghdr *genl_hdr = (genlmsghdr *)nlmsg_data(nlh);
    struct nlattr *attrs[ATTR_MAX + 1];

    int ret = genlmsg_parse(nlh, 0, attrs, ATTR_MAX, nullptr);
    if (ret < 0) {
        std::cerr << "Failed to parse Generic Netlink message!" << std::endl;
        return ret;
    }

    // Выводим номер seq
    std::cout << "Received message with seq: " << nlh->nlmsg_seq << std::endl;

    if (attrs[ATTR_MSG]) {
        const char *data = nla_get_string(attrs[ATTR_MSG]);
        std::cout << "Received message from kernel: " << data << std::endl;
        auto result_json = process_request(data);
        std::cout << "result_json: " << result_json.dump() << std::endl;
        if (result_json.contains("error")) {
            std::cerr << "err: " << result_json.at("error") << std::endl;
            return NL_OK;
        }
        if (result_json.contains("result")) {
            send_message(sock, family_id, COMMAND_SERVER, result_json.dump());
        }

    } else {
        std::cerr << "Message has no payload!" << std::endl;
    }

    return NL_OK;
}

int main() {
    std::cout << "Starting server..." << std::endl;

    // Создание сокета
    sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate Netlink socket!" << std::endl;
        return -1;
    }

    // todo: наверное не самое хорошее решение но в данном случа оно не надо
    nl_socket_disable_seq_check(sock);

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

    if (!send_message(sock, family_id, COMMAND_SERVER, request_json.dump())) {
        std::cerr << "Failed to send message!" << std::endl;
        nl_socket_free(sock);
        return -1;
    }

    std::cout << "Waiting for responses from kernel..." << std::endl;
    while (true) {
        int ret1 = nl_recvmsgs_default(sock);
        if (ret1 < 0) {
            std::cerr << "Error receiving message: " << nl_geterror(ret1) << std::endl;
            break;  // Выйти из цикла в случае ошибки, если необходимо
        }
    }

    nl_socket_free(sock);
    std::cout << "Client finished." << std::endl;
    return 0;
}