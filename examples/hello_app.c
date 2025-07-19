#include "http_server.h"
#include "http_request.h"
#include "http_response.h"

void handle_hello(HttpRequest *req, HttpResponse *res) {
    http_response_send_json(res, 200, "{\"message\": \"Hello from C server\"}");
}

int main() {
    HttpServer* srv = http_server_new(8000);
    http_server_on(srv, "GET", "/hello", handle_hello); 
    http_server_run(srv);
    return 0;
}
