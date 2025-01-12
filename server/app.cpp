#include "server.hpp"

int main() {
    try {
        netlink::server::Server server;
        server.wait_for_response();
    } catch (std::exception &ex) {
        printf("Error: %s\n", ex.what());
        return -1;
    }
    return 0;
}