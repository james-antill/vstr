#ifndef EX_HTTPD_ERR_CODE_H
#define EX_HTTPD_ERR_CODE_H

#define CONF_LINE_RET_301 "Moved Permanently"
#define CONF_MSG_FMT_301 "%s${vstr:%p%zu%zu%u}%s"
#define CONF_MSG__FMT_301_BEG "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>301 Moved Permanently</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>301 Moved Permanently</h1>\r\n\
    <p>The document has moved <a href=\"\
"

#define CONF_MSG__FMT_301_END "\
\">here</a>.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_MSG_LEN_301(s1) (((s1)->len) +                             \
                              strlen(CONF_MSG__FMT_301_BEG) +           \
                              strlen(CONF_MSG__FMT_301_END))

#define CONF_LINE_RET_400 "Bad Request"
#define CONF_MSG_RET_400 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>400 Bad Request</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>400 Bad Request</h1>\r\n\
    <p>The request could not be understood.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_403 "Forbidden"
#define CONF_MSG_RET_403 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>403 Forbidden</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>403 Forbidden</h1>\r\n\
    <p>The request is forbidden.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_404 "Not Found"
#define CONF_MSG_RET_404 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>404 Not Found</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>404 Not Found</h1>\r\n\
    <p>The document requested was not found.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_405 "Method Not Allowed"
#define CONF_MSG_RET_405 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>405 Method Not Allowed</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>405 Method Not Allowed</h1>\r\n\
    <p>The method specified is not allowed.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_412 "Precondition Failed"
#define CONF_MSG_RET_412 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>412 Precondition Failed</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>412 Precondition Failed</h1>\r\n\
    <p>The precondition given in one or more of the request-header fields evaluated to false.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_414 "Request-URI Too Long"
#define CONF_MSG_RET_414 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>414 Request-URI Too Long</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>414 Request-URI Too Long</h1>\r\n\
    <p>The document request was too long.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_416 "Requested Range Not Satisfiable"
#define CONF_MSG_RET_416 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>416 Requested Range Not Satisfiable</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>416 Requested Range Not Satisfiable</h1>\r\n\
    <p>The document request range was not valid.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_417 "Expectation Failed"
#define CONF_MSG_RET_417 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>417 Expectation Failed</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>417 Expectation Failed</h1>\r\n\
    <p>The expectation given in an Expect request-header field could not be met by this server.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_500 "Internal Server Error"
#define CONF_MSG_RET_500 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>500 Internal Server Error</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>500 Internal Server Error</h1>\r\n\
    <p>The server had an error.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_501 "Not Implemented"
#define CONF_MSG_RET_501 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>501 Not Implemented</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>501 Not Implemented</h1>\r\n\
    <p>The request method is not implemented.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_LINE_RET_505 "Version not supported"
#define CONF_MSG_RET_505 "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n\
<html>\r\n\
  <head>\r\n\
    <title>505 Version not supported</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>505 Version not supported</h1>\r\n\
    <p>The version of http used is not supported.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define HTTPD_ERR(req, code) do {                                    \
      req->http_error_code  = (code);                                \
      req->http_error_line  = CONF_LINE_RET_ ## code ;               \
      req->http_error_len   = strlen( CONF_MSG_RET_ ## code );       \
      if (!req->head_op)                                             \
        req->http_error_msg = CONF_MSG_RET_ ## code ;                \
    } while (0)

#define HTTPD_ERR_RET_VAL(req, code, val) do {          \
      HTTPD_ERR(req, code);                             \
      return (val);                                     \
    } while (0)

#define HTTPD_ERR_GOTO(req, code, label) do {           \
      HTTPD_ERR(req, code);                             \
      goto label ;                                      \
    } while (0)

/* doing http://www.example.com/foo/bar where bar is a dir is bad
   because all relative links will be relative to foo, not bar
*/
#define HTTP_REQ_CHK_DIR(req, goto_label) do {                          \
      if (VSUFFIX((req)->fname, 1, (req)->fname->len, "/.."))           \
      {                                                                 \
        HTTPD_ERR(req, 403);                                            \
        goto goto_label ;                                               \
      }                                                                 \
      else if (!VSUFFIX((req)->fname, 1, (req)->fname->len, "/"))       \
      {                                                                 \
        vstr_del((req)->fname, 1, req->vhost_prefix_len);               \
        req->vhost_prefix_len = 0;                                      \
                                                                        \
        vstr_conv_encode_uri((req)->fname, 1, (req)->fname->len);       \
        vstr_add_cstr_buf((req)->fname, (req)->fname->len, "/");        \
        req->http_error_code = 301;                                     \
        req->http_error_line = CONF_LINE_RET_301;                       \
        req->http_error_len  = CONF_MSG_LEN_301((req)->fname);          \
        if (!req->head_op)                                              \
          req->redirect_http_error_msg = TRUE;                          \
        goto goto_label ;                                               \
      }                                                                 \
      else                                                              \
        vstr_add_cstr_ptr((req)->fname, (req)->fname->len, "index.html"); \
    } while (0)
      
#endif
