#ifndef EX_HTTPD_ERR_CODE_H
#define EX_HTTPD_ERR_CODE_H

#define CONF_MSG_RET_400 "\
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

#define CONF_MSG_RET_401 "\
<html>\r\n\
  <head>\r\n\
    <title>401 Unauthorized</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>401 Unauthorized</h1>\r\n\
    <p>The request requires authorization.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define CONF_MSG_RET_403 "\
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

#define CONF_MSG_RET_404 "\
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

#define CONF_MSG_RET_414 "\
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

#define CONF_MSG_RET_500 "\
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

#define CONF_MSG_RET_501 "\
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

#define CONF_MSG_RET_505 "\
<html>\r\n\
  <head>\r\n\
    <title>505 Version</title>\r\n\
  </head>\r\n\
  <body>\r\n\
    <h1>505 Version</h1>\r\n\
    <p>The version of http used is not supported.</p>\r\n\
  </body>\r\n\
</html>\r\n\
"

#define HTTPD_ERR(code, skip_msg) do {          \
      http_error_code = (code);                 \
      if (!(skip_msg))                          \
      http_error_msg  = CONF_MSG_RET_ ## code ; \
    } while (0)

#endif
