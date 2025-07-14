#include "http_server.h"
#include "request.h"

void handle_hello(Request *req, Response *res) {
    res_send_json(res, 200, "{\"message\": \"Hello from C server\"}");
}

int main() {
    http_register_handler("GET", "/hello", handle_hello);
    http_server_listen(8000);
    return 0;
}
