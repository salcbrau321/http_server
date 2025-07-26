#include <criterion/criterion.h>
#include <string.h>
#include "http_request_parser.h"
#include "http_request.h"
#include "test_helper.h"

Test(HelloSuite, sanity) {
    cr_assert_str_eq("foo", "foo", "Criterion should be working");
}

Test(HttpParser, acccepts_valid_method_GET) {
    const char* raw = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    ParseResult r = parse_request(raw); 

    cr_assert_eq(r.status, PARSE_OK, "Expected status PARSE_OK, got %d", r.status);
    cr_assert_str_eq(r.request->method, "POST", "Expected method \"GET\", got \"%s\"", r.request->method);

    http_request_free(r.request);
}

