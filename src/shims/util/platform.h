#pragma once

// Polyfill header to expose libobs/utils/platform.h functions 
// that are not part of the libobs public headers

extern char *os_get_abs_path_ptr(const char *path);
extern char *os_quick_read_utf8_file(const char *path);
