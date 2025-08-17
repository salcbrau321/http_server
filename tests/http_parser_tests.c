#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>
#include "http_request_parser.h"
#include "http_request.h"
#include "test_helper.h"

static void assert_streq(const char* actual, const char* expected, const char* what) {
    cr_assert_not_null(actual, "%s should not be NULL", what);
    cr_assert_not_null(expected, "test bug: expected string for %s is NULL", what);
    cr_assert(strcmp(actual, expected) == 0,
              "%s mismatch:\n  expected: `%s`\n  actual:   `%s`",
              what, expected, actual);
}

Test(HelloSuite, Sanity) {
    cr_assert_str_eq("foo", "foo", "Criterion should be working");
}

Test(HttpParser_Method, ParseMethod_ValidTokens_StaysInMethodUntilSpace) {
    const char* good[] = {
        "GET", "POST", "PUT", "DELETE",
        "PATCH", "HEAD", "OPTIONS",
        "FOO123", "MKACTIVITY", "MAX_LENGTH_16___"
    };
    for (size_t i = 0; i < sizeof(good)/sizeof(*good); i++) {
        HttpRequestParser* p = http_request_parser_new();

        ParseResult r = http_request_parser_execute(p, good[i], strlen(good[i]));

        cr_assert_eq(
            r.status,
            PARSE_INCOMPLETE,
            "Expected status PARSE_OK - %d, got status %d for %s",
            PARSE_INCOMPLETE,
            r.status,
            good[i]);

        cr_assert_eq(
            r.parser_state,
            HTTP_STATE_METHOD,
            "Expected parser_state HTTP_STATE_METHOD - %d, got parser_state %d for %s", 
            HTTP_STATE_METHOD,
            r.parser_state,
            good[i]);

        http_request_free(r.request);
        r = http_request_parser_execute(p, " ", 1);
        
        cr_assert_eq(
            r.status,
            PARSE_INCOMPLETE,
            "Expected status PARSE_OK - %d, got status %d for %s (space feed) ",
            PARSE_INCOMPLETE,
            r.parser_state,
            good[i]);

        cr_assert_eq(
            r.parser_state,
            HTTP_STATE_TARGET,
            "Expected parser_state HTTP_STATE_TARGET - %d, got parser_state %d for %s (space feed)", 
            HTTP_STATE_TARGET,
            r.parser_state,
            good[i]);
        
        http_request_free(r.request);
        free(p);
    }
}

Test(HttpParser_Method, ParseMethod_InvalidTokens_RequestError) {
    const char *bad[] = {
        "\r\n", "", " GET", "GE T", "G*ET",                  
        "TOO_LONG_METHOD_NAME_EXCEEDS_CAP"  
    };
    for (size_t i = 0; i < sizeof(bad)/sizeof(*bad); i++) {
        HttpRequestParser* p = http_request_parser_new();

        ParseResult r = http_request_parser_execute(p, bad[i], strlen(bad[i]));

        cr_assert_eq(
            r.status,
            PARSE_ERROR_REQUEST, 
            "Expected status PARSE_ERROR_REQUEST - %d, got status %d for %s",
            PARSE_ERROR_REQUEST,
            r.status,
            bad[i]);

        http_request_free(r.request);
        free(p);
    }
}

Test(HttpParser_EndToEnd, Parser_Get_TwoChunks_Completes) {
    const char* msg = 
        "GET /cats/1 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: test-client\r\n"
        "\r\n";

    size_t total = strlen(msg);
    size_t split = 20;
    const char* part1 = msg;
    size_t len1 = split;
    const char* part2 = msg + split;
    size_t len2 = total - split;
    
    HttpRequestParser* p = http_request_parser_new();
    cr_assert_not_null(p);

    ParseResult r1 = http_request_parser_execute(p, part1, len1);
    cr_assert_neq(
        r1.status,
        PARSE_OK,
        "Parser marked COMPLETED too early after first chunk");
    cr_assert_not_null(
        p->request,
        "Parser should have allocated a request");
    cr_assert_neq(
        p->state,
        HTTP_STATE_IDLE,
        "Parser should not be idle during parse");

    ParseResult r2 = http_request_parser_execute(p, part2, len2);
    cr_assert_eq(
        r2.status, 
        PARSE_OK,
        "Parser failed to complete after second chunk");
    cr_assert_not_null(r2.request, "ParseResult.request must be set on COMPLETE");
    cr_assert_null(p->request, "Parser request pointer should be NULL after handoff");
    cr_assert_eq(
        p->state,
        HTTP_STATE_IDLE,
        "Parser should return to IDLE after completion");

    assert_streq(r2.request->method, "GET", "method");
    assert_streq(r2.request->path, "/cats/1", "path (before query)");
    // cr_assert_eq(r2.request->query_count, 1, "expected one query param");
    //assert_streq(r2.request->query_name[0], "name", "query param name");
    //assert_streq(r2.request->query_val[0], "world", "query param value");
    assert_streq(r2.request->host, "example.com", "Host header");

    http_request_free(r2.request);
    free(p);
}


Test(HttpParser_EndToEnd, Parser_Get_Pipeline_TwoInSecondChunk) {
    const char *req1 =
        "GET /one?x=1 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: t\r\n"
        "X-Trim:    v   \r\n"
        "\r\n";

    const char *req2 =
        "GET /two HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    size_t len1 = strlen(req1);
    size_t half = len1 / 2;

    size_t len2 = (len1 - half) + strlen(req2);
    char *chunk2 = malloc(len2);
    memcpy(chunk2, req1 + half, len1 - half);
    memcpy(chunk2 + (len1 - half), req2, strlen(req2));

    HttpRequestParser *p = http_request_parser_new();

    ParseResult a = http_request_parser_execute(p, req1, half);
    cr_assert_neq(a.status, PARSE_OK);

    size_t off = 0;

    ParseResult b = http_request_parser_execute(p, chunk2 + off, len2 - off);
    cr_assert_eq(b.status, PARSE_OK);
    cr_assert(b.bytes_consumed > 0 && b.bytes_consumed <= len2 - off);
    assert_streq(b.request->path, "/one", "first path");
    http_request_free(b.request);
    off += b.bytes_consumed;

    ParseResult c = http_request_parser_execute(p, chunk2 + off, len2 - off);
    cr_assert_eq(c.status, PARSE_OK);
    assert_streq(c.request->path, "/two", "second path");
    http_request_free(c.request);

    free(chunk2);
    free(p);
}
