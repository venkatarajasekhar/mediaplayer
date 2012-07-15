#include "stddef.h"

extern int memcmp(const void *s1, const void *s2, size_t n) __attribute__ ((__pure__));
extern int strncmp(const char *s1, const char *s2, size_t n) __attribute__ ((__pure__));
extern unsigned long int strtoul(const char *nptr, char **endptr, int base) __attribute__ ((__pure__));
extern size_t strlen(const char *s) __attribute__ ((__pure__));
extern char *strchr(const char *s, int c) __attribute__ ((__pure__));
