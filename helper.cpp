#include "helper.h"

#include <iostream>
#include <set>
#include <algorithm>
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <memory>

#include <sstream>


#include <fcntl.h>
#include <sys/epoll.h>

// дескрипторы не передаются, а передается пачка информации по сокету и к ней можно прикрепить дескриптор 
ssize_t sock_fd_write(int sock, //сокет в который будем посылать
 void * buf, // информация котрую будемы посылать
 ssize_t buflen, // длина 
int fd // приклеиваем к информации (буфферу) один дескриптор
){
    ssize_t     size;
    struct msghdr   msg; // правильно её заполнить - главная задача
    struct iovec    iov; // наш буфер
	   
 union {
        struct cmsghdr  cmsghdr;
        char        control[CMSG_SPACE(sizeof (int))];
    } cmsgu; // здесь будем хранить наш дескриптор
    struct cmsghdr  *cmsg;
  //!!!! ВАЖНО ЧТОБЫ передать дескриптор мы должны передать хоть один байт служебной информации => buflen != 0
    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov; 
    msg.msg_iovlen = 1; // передаем ровно один буффер - это надо указать так как функция sendmessage может передатвать несколкьо буфферов разной длины склеивая иих в один

    if (fd != -1) {
     
        msg.msg_control = cmsgu.control; // служебна инфа
        msg.msg_controllen = sizeof(cmsgu.control); // служебная инфа

        cmsg = CMSG_FIRSTHDR(&msg); // первый макрос
        cmsg->cmsg_len = CMSG_LEN(sizeof (int)); // длина равна размеру int
        cmsg->cmsg_level = SOL_SOCKET; //  обязательно !!! указыает что передаем сокет
        cmsg->cmsg_type = SCM_RIGHTS;  // ОБяхательно!! то же что и выше 

   //     printf ("passing fd %d\n", fd);
        *((int *) CMSG_DATA(cmsg)) = fd; // сам дескриптор
    } else {
      /// передаем по сокету без дескриптора
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
 //       printf ("not passing fd\n");
    }
 
    /// ОТПРАВИЛИ!
    size = sendmsg(sock, &msg, 0); // функция похожа на sendTO

    if (size < 0)
        perror ("sendmsg");
    return size;
}


int set_nonblock(int fd){
	int flags;
#if defined(O_NONBLOCK)
	if (-1 == (flags = fcntl(fd,F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else 
	flags = 1;
	return ioctl(fd, FIONBIO, &flags);
#endif
}	

size_t
sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd)
{
    ssize_t     size;

    if (fd) {
        // то чже что  и во write(см первую строку
	struct msghdr   msg;
        struct iovec    iov;
        union {
            struct cmsghdr  cmsghdr;
            char        control[CMSG_SPACE(sizeof (int))];
        } cmsgu;
        struct cmsghdr  *cmsg;
	///
	
        iov.iov_base = buf;
        iov.iov_len = bufsize;
	//

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
      	//
	 size = recvmsg (sock, &msg, 0); // получаем сообщение
        if (size < 0) {
            perror ("recvmsg");
            exit(1);
        }
	// парсим 
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmsg->cmsg_level != SOL_SOCKET) {// проверяем что пришел sol_Scoket
                fprintf (stderr, "invalid cmsg_level %d\n",
                     cmsg->cmsg_level);
                exit(1);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS) { // проверяем что передаем на него права
                fprintf (stderr, "invalid cmsg_type %d\n",
                     cmsg->cmsg_type);
                exit(1);
            }

            *fd = *((int *) CMSG_DATA(cmsg)); // читаем файловый дескриптор:
            printf ("received fd %d\n", *fd);
        } else{
		*fd = -1; // значит ничего не пришло
	//	std::cout << "Nothing to in\n";
	}
            
    } else {
	// если файловый дескриптор нулл
        size = read (sock, buf, bufsize);
        if (size < 0) {
            perror("read");
            exit(1);
        }
    }
    return size;
}
