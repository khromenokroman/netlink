#include "client.hpp"

//@todo: не нравиться что логирование и исключение надо что то одно оставить, сделать красиво надо
netlink::client::Client::Client() {
    openlog("NetlinkClient", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Initializing the Netlink client");

    m_sock = nl_socket_alloc();
    if (!m_sock) {
        syslog(LOG_ERR, "Failed to allocate Netlink socket");
        throw std::runtime_error("Failed to allocate Netlink socket");
    }

    nl_socket_disable_seq_check(m_sock);

    if (genl_connect(m_sock)) {
        syslog(LOG_ERR, "Failed to establish a connection to Netlink");
        throw std::runtime_error("Failed to establish a connection to Netlink");
    }

    m_family_id = genl_ctrl_resolve(m_sock, M_FAMILY_NAME);
    if (m_family_id < 0) {
        syslog(LOG_ERR, "Failed to resolve the Netlink family name");
        throw std::runtime_error("Failed to resolve the Netlink family name");
    }

    nl_socket_modify_cb(m_sock, NL_CB_MSG_IN, NL_CB_CUSTOM, receive_message, nullptr);

    syslog(LOG_INFO, "Netlink client initialized successfully");
}

netlink::client::Client::~Client() {
    syslog(LOG_INFO, "Netlink client socket closed and resources released");
    //@todo: переделать на uniq
    nl_socket_free(m_sock);
    closelog();
}

void netlink::client::Client::send_request(const nlohmann::json &request_json) {
    auto deleter_msg = [](nl_msg *msg) {
        if (msg) {
            nlmsg_free(msg);
        }
    };
    std::unique_ptr<nl_msg, decltype(deleter_msg)> msg(nlmsg_alloc(), deleter_msg);

    syslog(LOG_DEBUG, "Sending request: %s", request_json.dump().c_str());

    if (!genlmsg_put(msg.get(), NL_AUTO_PORT, NL_AUTO_SEQ, m_family_id, 0, 0, M_COMMAND_CLIENT, 1)) {
        syslog(LOG_ERR, "Failed to create Netlink message header");
        throw std::runtime_error("Failed to create Netlink message header");
    }

    if (nla_put_string(msg.get(), static_cast<int>(ATTR::ATTR_MSG), request_json.dump().c_str())) {
        syslog(LOG_ERR, "Failed to attach JSON payload to Netlink message");
        throw std::runtime_error("Failed to attach JSON payload to Netlink message");
    }

    int ret = nl_send_auto(m_sock, msg.get());
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to send Netlink message");
        throw std::runtime_error("Failed to send Netlink message");
    } else {
        struct nlmsghdr *nlh = nlmsg_hdr(msg.get());
        syslog(LOG_INFO, "Message sent successfully with sequence number: %d", nlh->nlmsg_seq);
    }
}

void netlink::client::Client::wait_for_response() {
    syslog(LOG_INFO, "Waiting for responses from the kernel");
    while (true) {
        int ret = nl_recvmsgs_default(m_sock);
        if (ret < 0) {
            syslog(LOG_ERR, "Error while receiving message from kernel: %s", nl_geterror(ret));
            break;
        }
    }
    syslog(LOG_INFO, "Client operations completed");
}

int netlink::client::Client::receive_message(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[static_cast<int>(ATTR::ATTR_MAX) + 1];

    int ret = genlmsg_parse(nlh, 0, attrs, static_cast<int>(ATTR::ATTR_MAX), nullptr);
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to parse received Netlink message");
        return ret;
    }

    if (attrs[static_cast<int>(ATTR::ATTR_MSG)]) {
        const char *data = nla_get_string(attrs[static_cast<int>(ATTR::ATTR_MSG)]);
        syslog(LOG_INFO, "Received message: %s", data);
        printf("Received message: %s\n", data);
        exit(0);
    } else {
        syslog(LOG_DEBUG, "Received message with no payload");
    }

    return NL_OK;
}