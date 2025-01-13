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

/* для тестов, так себе решение */
namespace tests {
class ServerTest_Friend;
}

namespace netlink::server {

enum class ATTR : int {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};



class Server final {
   public:
    /**
     * @brief Конструктор класса Netlink-сервера.
     *
     * Инициализирует Netlink-сервер, выделяет сокет, устанавливает соединение с Netlink,
     * разрешает имя семейства Netlink и отправляет тестовое сообщение.
     *
     * @throw std::runtime_error Если не удалось выделить сокет, установить соединение или определить семейство.
     */
    Server();
    Server(Server const &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server const &) = delete;
    Server &operator=(Server &&) = delete;
    ~Server();
    friend class tests::ServerTest_Friend;
    /**
     * @brief Ожидает ответы от ядра.
     *
     * Этот метод блокируется и принимает входящие сообщения от ядра, обрабатывая их.
     * Останавливается в случае возникновения ошибки.
     */
    void wait_for_response();

   private:
    /**
     * @brief Обработчик для получения сообщений из подсистемы Netlink.
     *
     * Разбирает полученные сообщения, обрабатывает запросы и отправляет ответы
     * на основе содержимого сообщений.
     *
     * @note Этот метод регистрируется как callback для обработки входящих сообщений.
     *
     * @param msg Структура сообщения Netlink.
     * @param arg Пользовательский аргумент, содержащий указатель на текущий экземпляр Server.
     *
     * @return NL_OK при успешной обработке сообщения или код ошибки в противном случае.
     */
    static int receive_message(struct nl_msg *msg, void *arg);
    /**
     * @brief Отправляет сообщение Netlink в ядро.
     *
     * Создает сообщение Netlink с JSON-полезной нагрузкой и отправляет его.
     * Используется как для запросов, так и для ответов.
     *
     * @param payload JSON-строка, которая будет отправлена.
     *
     * @throw std::runtime_error Если невозможно создать сообщение, прикрепить JSON или отправить.
     */
    void send_message(const std::string &payload);
    /**
     * @brief Обрабатывает JSON-запрос.
     *
     * Разбирает JSON-запрос, проверяет наличие нужных полей, выполняет требуемое действие
     * и возвращает результат в формате JSON.
     *
     * @param request_json JSON-строка с запросом.
     *
     * @return JSON с вычисленным результатом или пустой JSON-объект для других сообщений.
     *
     * @throw std::runtime_error Если входной JSON некорректен или запрошено неподдерживаемое действие.
     */
    nlohmann::json process_request(std::string const &request_json);

    struct nl_sock *m_sock = nullptr;                                 // 8
    static constexpr const char *const M_FAMILY_NAME = "calc_family"; // 8
    void *data = nullptr;                                             // 8
    int m_family_id = 0;                                              // 4
    static constexpr int M_COMMAND_SERVER = 2;                        // 4
};

} // namespace netlink::server