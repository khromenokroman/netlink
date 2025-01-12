#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>

// #define FAMILY_NAME "calc_family"

enum {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};

static_assert(sizeof(int) == 4);

class Netlink final {
   public:
    Netlink() {
        printf("Starting server...\n");
        m_sock = nl_socket_alloc();
        if (!m_sock) {
            throw std::runtime_error("Failed to allocate Netlink socket");
        }

        // todo: наверное не самое хорошее решение но в данном случа оно не надо
        nl_socket_disable_seq_check(m_sock);

        if (genl_connect(m_sock)) {
            throw std::runtime_error("Failed to connect to Netlink");
        }

        // Получение ID семейства
        m_family_id = genl_ctrl_resolve(m_sock, M_FAMILY_NAME);
        if (m_family_id < 0) {
            throw std::runtime_error("Unable to resolve family");
        }

        // Регистрация обработчика сообщений
        nl_socket_modify_cb(m_sock, NL_CB_MSG_IN, NL_CB_CUSTOM, receive_message, this);

        // Создание JSON-запроса
        nlohmann::json request_json;
        request_json["message"] = "Hello";

        if (!send_message(request_json.dump())) {
            throw std::runtime_error("Failed to send message");
        }
    }
    ~Netlink() { nl_socket_free(m_sock); }

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

        // Проверяем и обрабатываем сообщение
        if (attrs[ATTR_MSG]) {
            const char *data = nla_get_string(attrs[ATTR_MSG]); // Локальная переменная
            std::cout << "Received message from kernel: " << data << std::endl;

            // Обрабатываем сообщение прямо здесь
            auto result_json = static_cast<Netlink *>(arg)->process_request(data);
            std::cout << "result_json: " << result_json.dump() << std::endl;

            // Если произошла ошибка
            if (result_json.contains("error")) {
                std::cerr << "err: " << result_json.at("error") << std::endl;
                return NL_OK;
            }

            // Если есть результат, отправляем его обратно
            if (result_json.contains("result")) {
                static_cast<Netlink *>(arg)->send_message(result_json.dump());
            }
        } else {
            std::cerr << "Message has no payload!" << std::endl;
        }

        return NL_OK;
    }
    bool send_message(const std::string &payload) {
        std::cout << "Sending message: " << payload << std::endl;
        nl_msg *msg = nlmsg_alloc();
        if (!msg) {
            std::cerr << "Failed to allocate message!" << std::endl;
            return false;
        }

        if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, m_family_id, 0, 0, M_COMMAND_SERVER, 1)) {
            std::cerr << "Failed to create Netlink message header!" << std::endl;
            nlmsg_free(msg);
            return false;
        }

        if (nla_put_string(msg, ATTR_MSG, payload.c_str())) {
            std::cerr << "Failed to attach JSON payload!" << std::endl;
            nlmsg_free(msg);
            return false;
        }

        int ret = nl_send_auto(m_sock, msg);
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
    nlohmann::json process_request(std::string const &request_json) {
        std::cout << "Processing request: " << request_json << std::endl;
        // Парсим JSON из строки
        nlohmann::json request = nlohmann::json::parse(request_json);

        // Проверяем наличие ключей "action", "arg1", "arg2"
        if (!request.contains("action") || !request.contains("arg1") || !request.contains("arg2")) {
            return nlohmann::json({{"error", "Invalid input. Missing fields 'action', 'arg1', or 'arg2'"}});
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
            return nlohmann::json({{"error", "Invalid action. Supported actions are 'add', 'sub', 'mul'"}});
        }

        // Формируем и возвращаем результат
        nlohmann::json response;
        response["result"] = result;
        return response;
    }
    void wait_for_response() {
        std::cout << "Waiting for responses from kernel..." << std::endl;
        while (true) {
            int ret = nl_recvmsgs_default(m_sock);
            if (ret < 0) {
                std::cerr << "Error receiving message: " << nl_geterror(ret) << std::endl;
                break;
            }
        }
        std::cout << "Client finished." << std::endl;
    }

   private:
    struct nl_sock *m_sock = nullptr;                                 // 8
    static constexpr const char *const M_FAMILY_NAME = "calc_family"; // 8
    void *data = nullptr;                                             // 8
    int m_family_id = 0;                                              // 4
    static constexpr int M_COMMAND_SERVER = 2;                        // 4
};

int main() {
    Netlink server;
    server.wait_for_response();

    return 0;
}