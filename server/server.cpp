#include "server.hpp"

//@todo: не нравиться что логирование и исключение надо что то одно оставить, сделать красиво надо
netlink::server::Server::Server() {
    openlog("NetlinkServer", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting the Netlink server");

    m_sock = nl_socket_alloc();
    if (!m_sock) {
        syslog(LOG_ERR, "Failed to allocate the Netlink socket");
        throw std::runtime_error("Failed to allocate the Netlink socket");
    }

    nl_socket_disable_seq_check(m_sock);

    if (genl_connect(m_sock)) {
        syslog(LOG_ERR, "Failed to establish a connection to Netlink");
        throw std::runtime_error("Failed to establish a connection to Netlink");
    }

    m_family_id = genl_ctrl_resolve(m_sock, M_FAMILY_NAME);
    if (m_family_id < 0) {
        syslog(LOG_ERR, "Failed to resolve the Netlink family");
        throw std::runtime_error("Failed to resolve the Netlink family");
    }

    nl_socket_modify_cb(m_sock, NL_CB_MSG_IN, NL_CB_CUSTOM, receive_message, this);

    //@todo: не было полного описания задачи, поэтому пока так
    nlohmann::json request_json;
    request_json["message"] = "Hello";

    send_message(request_json.dump());
}

netlink::server::Server::~Server() {
    syslog(LOG_INFO, "Shutting down the Netlink server");
    //@todo: переделать на uniq
    nl_socket_free(m_sock);
    closelog();
}

int netlink::server::Server::receive_message(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[static_cast<int>(ATTR::ATTR_MAX) + 1];

    int ret = genlmsg_parse(nlh, 0, attrs, static_cast<int>(ATTR::ATTR_MAX), nullptr);
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to parse Generic Netlink message");
        return ret;
    }
    syslog(LOG_INFO, "Message received with sequence number: %d", nlh->nlmsg_seq);

    if (attrs[static_cast<int>(ATTR::ATTR_MSG)]) {
        const char *data = nla_get_string(attrs[static_cast<int>(ATTR::ATTR_MSG)]);
        syslog(LOG_INFO, "Message received from kernel: %s", data);

        nlohmann::json result_json{};
        try {
            result_json = static_cast<Server *>(arg)->process_request(data);
            if (result_json.empty()) {
                syslog(LOG_DEBUG, "Processed JSON is empty");
                return NL_OK;
            }
        } catch (std::exception &ex) {
            syslog(LOG_ERR, "Error occurred: %s", ex.what());
            //@todo@: тут может быть проблема, для быстроты реализации пока так
            static_cast<Server *>(arg)->send_message(ex.what());
            return NL_OK;
        }

        if (result_json.contains("result")) {
            static_cast<Server *>(arg)->send_message(result_json.dump());
        }
    } else {
        syslog(LOG_DEBUG, "Message with sequence number %d has no payload", nlh->nlmsg_seq);
    }

    return NL_OK;
}

void netlink::server::Server::send_message(const std::string &payload) {
    syslog(LOG_DEBUG, "Sending message: %s", payload.c_str());

    auto deleter_msg = [](nl_msg *msg) {
        if (msg) {
            nlmsg_free(msg);
        }
    };
    std::unique_ptr<nl_msg, decltype(deleter_msg)> msg(nlmsg_alloc(), deleter_msg);

    if (!genlmsg_put(msg.get(), NL_AUTO_PORT, NL_AUTO_SEQ, m_family_id, 0, 0, M_COMMAND_SERVER, 1)) {
        syslog(LOG_ERR, "Failed to create Netlink message header");
        throw std::runtime_error("Failed to create Netlink message header");
    }

    if (nla_put_string(msg.get(), static_cast<int>(ATTR::ATTR_MSG), payload.c_str())) {
        syslog(LOG_ERR, "Failed to attach the JSON payload");
        throw std::runtime_error("Failed to attach the JSON payload");
    }

    int ret = nl_send_auto(m_sock, msg.get());
    if (ret < 0) {
        syslog(LOG_ERR, "Failed to send message");
        throw std::runtime_error("Failed to send message");
    } else {
        nlmsghdr *nlh = nlmsg_hdr(msg.get());
        syslog(LOG_DEBUG, "Message sent successfully with sequence number: %d", nlh->nlmsg_seq);
    }
}

nlohmann::json netlink::server::Server::process_request(std::string const &request_json) {
    syslog(LOG_DEBUG, "Processing the request: %s", request_json.c_str());
    nlohmann::json request = nlohmann::json::parse(request_json);
    if (request.contains("message")) {
        return {};
    }
    if (!request.contains("action") || !request.contains("arg1") || !request.contains("arg2")) {
        throw std::runtime_error("Invalid input. Missing fields 'action', 'arg1', or 'arg2'");
    }

    std::string action = request.at("action").get<std::string>();
    int arg1 = request.at("arg1").get<int>();
    int arg2 = request.at("arg2").get<int>();
    int result = 0;

    if (action == "add") {
        result = arg1 + arg2;
    } else if (action == "sub") {
        result = arg1 - arg2;
    } else if (action == "mul") {
        result = arg1 * arg2;
    } else {
        throw std::runtime_error("Invalid action. Supported actions are 'add', 'sub', 'mul'");
    }

    nlohmann::json response;
    response["result"] = result;
    return response;
}

void netlink::server::Server::wait_for_response() {
    syslog(LOG_DEBUG, "Waiting for responses from the kernel");
    while (true) {
        int ret = nl_recvmsgs_default(m_sock);
        if (ret < 0) {
            syslog(LOG_ERR, "An error occurred while receiving the message: %s", nl_geterror(ret));
            break;
        }
    }
    syslog(LOG_DEBUG, "Client operations completed");
}
