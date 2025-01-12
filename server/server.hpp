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

namespace netlink::server {

enum class ATTR : int {
    ATTR_UNSPEC,
    ATTR_MSG,
    ATTR_MAX,
};

class Server final {
   public:
    Server();
    Server(Server const &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server const &) = delete;
    Server &operator=(Server &&) = delete;
    ~Server();

    static int receive_message(struct nl_msg *msg, void *arg);
    void wait_for_response();

   private:
    void send_message(const std::string &payload);
    nlohmann::json process_request(std::string const &request_json);

    struct nl_sock *m_sock = nullptr;                                 // 8
    static constexpr const char *const M_FAMILY_NAME = "calc_family"; // 8
    void *data = nullptr;                                             // 8
    int m_family_id = 0;                                              // 4
    static constexpr int M_COMMAND_SERVER = 2;                        // 4
};

} // namespace netlink::server