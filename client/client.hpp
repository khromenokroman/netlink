#pragma once
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <syslog.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>

static_assert(sizeof(int) == 4);

//@todo: по хорошему надо сделать свои исключения
//@todo: по хорошему надо сделать свой логгер

namespace netlink::client {

enum class ATTR : int {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};

class Client final {
   public:
    /**
     * @brief Конструктор клиента Netlink.
     *
     * Инициализирует клиент Netlink: выделяет сокет, устанавливает соединение,
     * разрешает имя семейства Netlink и настраивает callback для обработки сообщений.
     *
     * @throw std::runtime_error Если не удалось выделить сокет, установить соединение или разрешить имя семейства.
     */
    Client();
    Client(Client const &) = delete;
    Client(Client &&) = delete;
    Client &operator=(Client const &) = delete;
    Client &operator=(Client &&) = delete;
    ~Client();
    /**
     * @brief Отправляет запрос в Netlink.
     *
     * Создает сообщение с указанной JSON-полезной нагрузкой и отправляет его.
     *
     * @param request_json JSON-объект с запросом.
     *
     * @throw std::runtime_error Если не удалось создать сообщение, прикрепить данные, или если отправка завершилась ошибкой.
     */
    void send_request(const nlohmann::json &request_json);
    /**
     * @brief Ожидает ответы от Netlink.
     *
     * Блокирующий метод, который слушает входящие сообщения от Netlink
     * до возникновения ошибки или завершения работы.
     */
    void wait_for_response();

   private:
    /**
     * @brief Callback для получения сообщений от Netlink.
     *
     * Обрабатывает входящее сообщение: разбирает его, получает полезную нагрузку и выводит её в лог.
     *
     * @note Callback выполняется автоматически при получении сообщения.
     *
     * @param msg Указатель на сообщение Netlink.
     * @param arg Дополнительный аргумент, который можно использовать при обработке (не используется здесь).
     *
     * @return NL_OK при успешной обработке сообщения или код ошибки в противном случае.
     */
    static int receive_message(struct nl_msg *msg, void *arg);

    struct nl_sock *m_sock = nullptr;                                 // 8
    static constexpr const char *const M_FAMILY_NAME = "calc_family"; // 8
    static constexpr int M_COMMAND_CLIENT = 1;                        // 4
    int m_family_id = 0;                                              // 4
};

} // namespace netlink::client