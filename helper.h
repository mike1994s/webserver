#ifndef HELPER_SOCK_READ_WRITE_NONBLOCK
#define HELPER_SOCK_READ_WRITE_NONBLOCK

#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
// дескрипторы не передаются, а передается пачка информации по сокету и к ней можно прикрепить дескриптор 
ssize_t sock_fd_write(int sock, //сокет в который будем посылать
 void * buf, // информация котрую будемы посылать
 ssize_t buflen, // длина 
int fd // приклеиваем к информации (буфферу) один дескриптор
);

int set_nonblock(int fd);


size_t
sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);



#endif
