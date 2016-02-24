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
#include <array>
#include <sstream>

#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>
#include "helper.h"
#define BYTES 1024
#define MAX_EVENTS 32
 

void respond(int fd)
{
    char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, fileDesc, bytes_read;

    memset( (void*)mesg, (int)'\0', 99999 );
    const char *ROOT = "/home/misha/web_server/";
    int RecvResult = recv(fd,mesg, 99999, MSG_NOSIGNAL);
		if (RecvResult == 0 && errno != EAGAIN){
			shutdown(fd,SHUT_RDWR);
			close(fd);
		}else if (RecvResult >0){
  
        	printf("%s", mesg);
        	reqline[0] = strtok (mesg, " \t\n"); // split on lexemes
       		 if ( strncmp(reqline[0], "GET\0", 4)==0 )  // if first 4 character equal
        	{
            		reqline[1] = strtok (NULL, " \t");
            		reqline[2] = strtok (NULL, " \t\n");
			std::cout << "reqline 1 " << reqline[1] << std::endl;
			std::cout << "reqline 2 " << reqline[2] << std::endl;
			    if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 
				&& strncmp(reqline[2], "HTTP/1.1", 8 ) !=0 )
			    {
				write(fd, "HTTP/1.0 400 Bad Request\n", 25);
			    }
			    else
			    {
                	if ( strncmp(reqline[1], "/\0", 2)==0 )
                    	reqline[1] = "/index.html";      
			
                	strcpy(path, ROOT);
                	strcpy(&path[strlen(ROOT)], reqline[1]);
                	printf("file: %s\n", path);

                	if ( (fileDesc=open(path, O_RDONLY))!=-1 )     
                	{
                    		send(fd, "HTTP/1.0 200 OK\n\n", 17, 0);
                    		while ( (bytes_read=read(fileDesc, data_to_send, BYTES))>0 )
                        		write (fd, data_to_send, bytes_read);
                	}
               			 else    write(fd, "HTTP/1.0 404 Not Found\n", 23);  
            		}
        }
    }
      shutdown(fd,SHUT_RDWR);
      close(fd);
}

void child(int sock)
{
    int fd;
    char    buf[16];
    ssize_t size;

    sleep(1);
    for (;;) {

        size = sock_fd_read(sock, buf, sizeof(buf), &fd);
        if (size <= 0)
            break;

        if (fd != -1) { 
		respond(fd);
   	  }
    }
	printf("child processes is end\n");
}

struct Descriptors{
	int sv[2]; 
};

class Parent{
public:
	static Parent& getInstance(){
		static Parent instance;
		return instance;
	}
 	Parent(Parent const&)         = delete;
        void operator=(Parent const&)  = delete;
	void addFd(int fd){
		m_fd.push_back(fd);
	};
	void run() {
		startServer();
		size_t index = 0;
		while(true){
			struct epoll_event Events[MAX_EVENTS];
			int N = epoll_wait(m_epoll, Events, MAX_EVENTS, -1);
		
			for (size_t i =0; i < N; ++i){
				if (Events[i].events & EPOLLHUP){
					 epoll_ctl(m_epoll, EPOLL_CTL_DEL, Events[i].data.fd, &(Events[i]));
					 shutdown(Events[i].data.fd,SHUT_RDWR);
					
		                	 close(Events[i].data.fd);
					 continue;
				}else {

					if (Events[i].data.fd == m_masterSocket) {
						handleConnection();
				
					}else {
						char * arg = "1";
						ssize_t size = sock_fd_write(m_fd[index], arg, 1,Events[i].data.fd);
						index = (1+index) % m_fd.size();
					}
				}		
		}
	}
		
	}
private:
	Parent(){
		m_numCpu = sysconf(_SC_NPROCESSORS_ONLN);
	}
	void startServer(){
		m_masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	 
		struct sockaddr_in SockAddr;
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_port = htons( 11141);
		SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		bind(m_masterSocket, (struct sockaddr *)(&SockAddr), sizeof(SockAddr));
	
		set_nonblock(m_masterSocket);
		listen(m_masterSocket, SOMAXCONN);
		m_epoll = epoll_create1(0);
		struct epoll_event Event;
		Event.data.fd = m_masterSocket;
		Event.events = EPOLLIN | EPOLLRDHUP;
	
		epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_masterSocket, &Event);
	}
	void handleConnection(){
		int SlaveSocket = accept(m_masterSocket, 0, 0);
		set_nonblock(SlaveSocket);
		struct epoll_event Event;
		Event.data.fd = SlaveSocket;
		Event.events = EPOLLIN | EPOLLRDHUP;
				
		epoll_ctl(m_epoll, EPOLL_CTL_ADD, SlaveSocket, &Event);
	}
	int m_epoll; 
	int m_masterSocket;
	int m_numCpu;
	std::vector<int> m_fd;
};

void parent(int sock){
	Parent::getInstance().addFd(sock);
}

int main(int argc, char **argv){
	int numCpu = sysconf(_SC_NPROCESSORS_ONLN);
 
	std::vector<Descriptors> desc;
	desc.resize(numCpu);
 
	bool isParent = true;
	for (int i  = 0; i < numCpu && isParent; ++i){
		std::cout << "pid my is = " << getpid() <<std::endl;
		int sv[2];
		  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
     			   perror("socketpair");
      			  exit(1);
    		}		
		pid_t forkId = fork();
		switch (forkId){
			case 0:{
				isParent = false;
				close(sv[0]);
        			child(sv[1]);
        			break;
			}	
			  case -1:
     				   perror("fork");
        				exit(1);
    			default:
        			close(sv[1]);
        			parent(sv[0]);
        			break;
    		}	
	}
	
	if (isParent){
 
		Parent::getInstance().run();
		int status;
		waitpid(-1, &status, 0);

	}
}	
