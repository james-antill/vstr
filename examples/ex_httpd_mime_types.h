#ifndef EX_HTTPD_MIME_TYPES_H
#define EX_HTTPD_MIME_TYPES_H

#define HTTP__REQ_MT_END(s1, var, mt, end)                              \
    else if (((s1)->len >= strlen(end)) &&                              \
             vstr_cmp_case_eod_cstr_eq((s1), 1, (s1)->len, end))        \
      (var) = (mt)

/* should be in a file -- so it's configurable, also allow overloads */
#define HTTP_REQ_MIME_TYPE(s1, var) do {                                \
      if (0) { }                                                        \
 HTTP__REQ_MT_END(s1, var, "application/x-tar",           ".tar");      \
 HTTP__REQ_MT_END(s1, var, "application/x-tar",           ".tar.gz");   \
 HTTP__REQ_MT_END(s1, var, "application/x-tar",           ".tar.bz2");  \
 HTTP__REQ_MT_END(s1, var, "application/x-gzip",          ".gz");       \
 HTTP__REQ_MT_END(s1, var, "application/pdf",             ".pdf");      \
 HTTP__REQ_MT_END(s1, var, "application/postscript",      ".ps");       \
 HTTP__REQ_MT_END(s1, var, "application/x-redhat-package-manager", ".rpm"); \
 HTTP__REQ_MT_END(s1, var, "audio/mpeg3",                 ".mp3");      \
 HTTP__REQ_MT_END(s1, var, "audio/ogg",                   ".ogg");      \
 HTTP__REQ_MT_END(s1, var, "audio/wav",                   ".wav");      \
 HTTP__REQ_MT_END(s1, var, "image/x-icon",                ".ico");      \
 HTTP__REQ_MT_END(s1, var, "image/gif",                   ".gif");      \
 HTTP__REQ_MT_END(s1, var, "image/jpeg",                  ".jpeg");     \
 HTTP__REQ_MT_END(s1, var, "image/jpeg",                  ".jpg");      \
 HTTP__REQ_MT_END(s1, var, "image/png",                   ".png");      \
 HTTP__REQ_MT_END(s1, var, "image/tiff",                  ".tif");      \
 HTTP__REQ_MT_END(s1, var, "image/tiff",                  ".tiff");     \
 HTTP__REQ_MT_END(s1, var, "text/comma-separated-values", ".csv");      \
 HTTP__REQ_MT_END(s1, var, "text/css",                    ".css");      \
 HTTP__REQ_MT_END(s1, var, "text/plain",                  "README");    \
 HTTP__REQ_MT_END(s1, var, "text/plain",                  "Makefile");  \
 HTTP__REQ_MT_END(s1, var, "text/plain",                  ".txt");      \
 HTTP__REQ_MT_END(s1, var, "text/plain",                  ".c");        \
 HTTP__REQ_MT_END(s1, var, "text/plain",                  ".h");        \
 HTTP__REQ_MT_END(s1, var, "text/html",                   ".htm");      \
 HTTP__REQ_MT_END(s1, var, "text/html",                   ".html");     \
 HTTP__REQ_MT_END(s1, var, "text/richtext",               ".rtx");      \
 HTTP__REQ_MT_END(s1, var, "text/rtf",                    ".rtf");      \
 HTTP__REQ_MT_END(s1, var, "text/sgml",                   ".sgml");     \
 HTTP__REQ_MT_END(s1, var, "text/sgml",                   ".sgm");      \
 HTTP__REQ_MT_END(s1, var, "text/tab-separated-values",   ".tsv");      \
 HTTP__REQ_MT_END(s1, var, "text/xml",                    ".xml");      \
 HTTP__REQ_MT_END(s1, var, "video/mpeg",                  ".mpeg");     \
 HTTP__REQ_MT_END(s1, var, "video/mpeg",                  ".mpg");      \
 HTTP__REQ_MT_END(s1, var, "video/mpeg",                  ".mpe");      \
 HTTP__REQ_MT_END(s1, var, "video/msvideo",               ".avi");      \
 HTTP__REQ_MT_END(s1, var, "video/quicktime",             ".mov");      \
 HTTP__REQ_MT_END(s1, var, "video/quicktime",             ".qt");       \
 else (var) = "application/octet-stream";                               \
    } while (0)

#endif
