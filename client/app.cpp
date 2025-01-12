#include "client.hpp"

int main() {
    try {
        netlink::client::Client client;

        nlohmann::json request_add;
        request_add["action"] = "add";
        request_add["arg1"] = 42;
        request_add["arg2"] = 58;

        nlohmann::json request_mul;
        request_mul["action"] = "mul";
        request_mul["arg1"] = 10;
        request_mul["arg2"] = 10;

        nlohmann::json request_sub;
        request_sub["action"] = "sub";
        request_sub["arg1"] = -59;
        request_sub["arg2"] = 100;

        nlohmann::json request_err;
        request_err["action"] = "sub1";
        request_err["arg1"] = -59;
        request_err["arg2"] = 100;

        // client.send_request(request_add);
        // client.send_request(request_mul);
        // client.send_request(request_sub);
        client.send_request(request_err);
        client.wait_for_response();
    } catch (const std::exception &e) {
        fprintf(stderr, "Error: %s", e.what());
        return -1;
    }

    return 0;
}