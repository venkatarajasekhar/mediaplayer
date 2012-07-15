#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

extern char *getenv(const char *name) __attribute__ ((__pure__));
extern char **environ;

extern void exit(int status) __attribute__((noreturn));

#endif
