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
    Client();
    Client(Client const &) = delete;
    Client(Client &&) = delete;
    Client &operator=(Client const &) = delete;
    Client &operator=(Client &&) = delete;
    ~Client();

    void send_request(const nlohmann::json &request_json);
    void wait_for_response();

   private:
    static int receive_message(struct nl_msg *msg, void *arg);

    struct nl_sock *m_sock = nullptr;                                 // 8
    static constexpr const char *const M_FAMILY_NAME = "calc_family"; // 8
    static constexpr int M_COMMAND_CLIENT = 1;                        // 4
    int m_family_id = 0;                                              // 4
};

} // namespace netlink::client