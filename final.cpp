#include <iostream>
#include <set>
#include <arpa/inet.h>
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
#include <sys/stat.h>

#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>
#include "helper.h"
#include <iostream>
#include <string>
#define BYTES 1024
#define MAX_EVENTS 32

char PORT[6];
std::string ROOT;
std::string IP;
int portNum = 35555; 
static const char not_found[] = "HTTP/1.0 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n";
static const char * CONTENT_TYPE = "Content-Type: text/html\r\n\r\n";
static const char* templ =  "Content-length: %d\r\n"

		       	   "Content-Type: text/html\r\n"

		       	   "\r\n"

		       	   "%s";
const char *HEADER = "HTTP/1.0 200 OK\r\n"; 
//char *HEADER_ANS = "HTTP/1.0 200 OK\n\nContent-length: \r\nContent-Type: text/html\r\n\r\n";
#include <sstream>
std::string DEFAULT_DOCUMENT = "/index.html";
class RequestServ{
	std::string method;
	std::string uri;
	std::string protocol;


public:
	std::string const &  getMethod()const {
		return method;
	}
	bool isValidProtocol()const{
		
		return (protocol == "HTTP/1.0");
	}
	std::string const & getUri()const{
		return uri;
	}
	void init(const char * message){
		std::string buffer(message);
		std::vector<std::string> tokens;
		size_t pos = 0;
		size_t newpos;
		while(pos != std::string::npos) {
    			newpos = buffer.find_first_of(" \t\n", pos);
    			tokens.push_back(buffer.substr(pos, newpos-pos));
    			if(pos != std::string::npos)
        			pos++;
		}
				
		if (tokens.size() > 2){
			method = tokens[0];
			uri = tokens[1];
			protocol = tokens[2];
		}
//		std::cout << method  << "  " << uri << " " << protocol << std::endl;
	}
	
};
void fillHeader(std::string & headerAns, int fd, const char * path ){
	int size;
	struct stat st;
	const char * p = path;
	stat(p, &st);
	size = st.st_size;
	std::stringstream strm;
	strm << "HTTP/1.0 200 OK\r\nContent-length: " ;
	strm << size << "\r\nContent-Type: text/html\r\n\r\n";
	headerAns = strm.str();
	
}
void respond(int fd)
{
    char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, fileDesc, bytes_read;

    memset( (void*)mesg, (int)'\0', 99999 );
   
    int RecvResult = recv(fd,mesg, 99999, MSG_NOSIGNAL); 
    if (RecvResult == 0 && errno != EAGAIN){
		shutdown(fd,SHUT_RDWR);
		close(fd);		
	//	std::cout << "error recv" << std::endl;
			return;
	}else if (RecvResult >0){ 		
        	reqline[0] = strtok (mesg, " \t\n"); // split on lexemes
       		 if ( strncmp(reqline[0], "GET\0", 4)==0 )  // if first 4 character equal
        	{
            		reqline[1] = strtok (NULL, " \t");
            		reqline[2] = strtok (NULL, " \t\n");
	 
			    if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 
				&& strncmp(reqline[2], "HTTP/1.1", 8 ) !=0 )
			    {
				send(fd, "HTTP/1.0 400 Bad Request\n", 25 , MSG_NOSIGNAL);
			    }
			    else
			    {

			if (strstr(reqline[1], "?") != NULL) {
				reqline[1] = strtok(reqline[1], "?");
			}
                	if ( strncmp(reqline[1], "/\0", 2)==0 )
                    		reqline[1] = "/index.html";      
			


			//reqline[1] = uri.c_str();
                	strcpy(path, ROOT.c_str());
                	strcpy(&path[strlen(ROOT.c_str())], reqline[1]);
   
                	if ( (fileDesc=open(path, O_RDONLY))!=-1 )     
                	{
				
				std::string header;
				fillHeader(header, fileDesc, path);
				send(fd,header.c_str(), (int)header.size(), MSG_NOSIGNAL);
                    		set_nonblock(fileDesc);
				while ( (bytes_read=read(fileDesc, data_to_send, BYTES))>0 )
                      			{
						if (bytes_read != -1)
							send (fd, data_to_send, bytes_read, MSG_NOSIGNAL);
                			}
			}
               		 else    send(fd, not_found, strlen(not_found), MSG_NOSIGNAL);  
            		}
      			shutdown(fd,SHUT_RDWR);
     		 	close(fd);
        }
    }else{
	}
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
				  if ((Events[i].events & EPOLLERR) ||
              (Events[i].events & EPOLLHUP) ||
              (!(Events[i].events & EPOLLIN)))
	    {
				
					 epoll_ctl(m_epoll, EPOLL_CTL_DEL, Events[i].data.fd, &(Events[i]));
					 shutdown(Events[i].data.fd,SHUT_RDWR);
					
		                	 close(Events[i].data.fd);
					 continue;
				}else {

					if (Events[i].data.fd == m_masterSocket) {
						handleConnection();
				
					}else {
						char  arg[1] = {'1'};
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
		SockAddr.sin_port = htons(std::stoi(PORT));
		inet_pton(AF_INET, IP.c_str(), &(SockAddr.sin_addr));
		//SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
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

int mainDaemon(int argc, char **argv){
	int numCpu = sysconf(_SC_NPROCESSORS_ONLN);
 	char c;
 	 while ((c = getopt (argc, argv, "h:p:d:")) != -1)
           switch (c)
        	{
            case 'd':
                ROOT = optarg;
               // strcpy(ROOT,optarg)	
//		std::cout << ROOT << std::endl;
                break;
            case 'p':
                strcpy(PORT,optarg);
//		std::cout << portNum << std::endl;
                break;
          case 'h': 
		IP = optarg;
//		std::cout << IP << std::endl; 
		break;
	   case '?':
                fprintf(stderr,"Wrong arguments given!!!\n");
                exit(1);
            default:
                exit(1);
        }
    
//    printf("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",ROOT.c_str(),"\033[0m");

	std::vector<Descriptors> desc;
	desc.resize(numCpu);
 
	bool isParent = true;
	for (int i  = 0; i < numCpu && isParent; ++i){
	//	std::cout << "pid my is = " << getpid() <<std::endl;
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
#include <cstdlib>
int main(int argc, char **argv){
	  /* Our process ID and Session ID */
        pid_t pid, sid;
        
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);
                
        /* Open any logs here */        
                
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        

        
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
	mainDaemon(argc, argv);
	 exit(EXIT_SUCCESS);
}	
