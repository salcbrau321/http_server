#include "http_server.h"
#include "http_request.h"
#include "http_response.h"

void handle_hello(HttpRequest *req, HttpResponse *res) {
    http_response_send_json(res, 200, "{\"message\": \"Hello from C server\"}");
}

void handle_cat(HttpRequest *req, HttpResponse *res) {
    http_response_send_json(res, 200, "{\"message\": \"meow meow\"}");
    if (req->query_count > 0) {
        for (int i = 0; i < req->query_count; i++) {
            printf("param %d: %s = %s\n",
                i,
                req->query_name[i],
                req->query_val[i]);
        }
    }
}

int main() {
    HttpServer* srv = http_server_new(8000);
    http_server_on(srv, "GET", "/hello", handle_hello); 
    http_server_on(srv, "GET", "/cats/:id", handle_cat); 
    http_server_on(srv, "POST", "/cats", handle_cat); 
    http_server_run(srv);
    return 0;
}
