#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <khc.h>
#include "secure_socket_impl.h"
#include "khc_impl.h"
#include "test_callbacks.h"

TEST_CASE( "HTTP Get" ) {
  khc http;

  khc_set_param(&http, KHC_PARAM_HOST, (char*)"api-jp.kii.com");
  khc_set_param(&http, KHC_PARAM_METHOD, (char*)"GET");
  khc_set_param(&http, KHC_PARAM_PATH, (char*)"");
  khc_set_param(&http, KHC_PARAM_REQ_HEADERS, NULL);

  ssl_context_t s_ctx;
  khc_set_cb_sock_connect(&http, s_connect_cb, &s_ctx);
  khc_set_cb_sock_send(&http, s_send_cb, &s_ctx);
  khc_set_cb_sock_recv(&http, s_recv_cb, &s_ctx);
  khc_set_cb_sock_close(&http, s_close_cb, &s_ctx);

  khct::cb::IOCtx io_ctx;
  khc_set_cb_read(&http, khct::cb::cb_read, &io_ctx);
  khc_set_cb_write(&http, khct::cb::cb_write, &io_ctx);
  khc_set_cb_header(&http, khct::cb::cb_header, &io_ctx);

  int on_read_called = 0;
  io_ctx.on_read = [=, &on_read_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_read_called;
    // No req body.
    return 0;
  };

  int on_header_called = 0;
  io_ctx.on_header = [=, &on_header_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_header_called;
    // Ignore resp headers.
    char str[size*count + 1];
    strncpy(str, buffer, size*count);
    str[size*count] = '\0';
    printf("%s\n", str);
    return size * count;
  };

  int on_write_called = 0;
  io_ctx.on_write = [=, &on_write_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_write_called;
    REQUIRE ( size == 1);
    REQUIRE ( count == 2 );
    REQUIRE ( strncmp(buffer, "{}", 2) == 0 );
    return size * count;
  };

  khc_code res = khc_perform(&http);
  REQUIRE( res == KHC_ERR_OK );
  REQUIRE( on_read_called == 1 );
  REQUIRE( on_header_called > 1 );
  REQUIRE( on_write_called == 1 );
}