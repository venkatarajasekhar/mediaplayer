#ifndef _UNISTD_H
#define _UNISTD_H

extern int execv(const char *path, char *const argv[]);
extern int execve(const char *filename, char *const argv [], char *const envp[]);
extern signed int fork(void);

#endif
