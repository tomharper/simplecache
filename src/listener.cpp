
/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <thread>
#include <memory>
#include "cache.h"


#define PORT "11211"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define MAX_BUFFER 1000

/*
typedef struct _header {
 unsigned char magic; // response/request 0x80/0x81
 unsigned char opcode; // get/set //  0x00    Get ,  0x01    Set
 unsigned short keylength; // should be 0?
 unsigned char extras;  // length in bytes of extra cmds - 0 here
 unsigned char data_type; // 0 - ignore...
 unsigned short reserved; // 0 - ignore...
 unsigned int total_body_len; // extra + key + value
 unsigned int opaque; // send back as is in response
 unsigned char cas[8];  // data version check
} header;
*/

unsigned char* parse_and_process(unsigned char* buf, int* len_out)
{
  int len = *len_out;
  bool valid = true;
  if (len < 24) {
    printf("invalid length\n");
    valid = false;
  }
  if (buf[0] != 0x80) {
    printf("invalid magic\n");
    valid = false;
  }
  if ((buf[1] != 0x00) && (buf[1] != 0x01)) {
    printf("invalid opcode\n");
    valid = false;
  }
  unsigned short keylen = (buf[2] << 8) | buf[3];
  if (keylen == 0) {
    printf("keylen invalid\n");
    valid = false;
  }

  // ignore byte 4- extra length
  // ignore byte 5- data type
  // ignore status/reserved byte 6,7-

  unsigned int totlen = (buf[11] << 24) | (buf[10] << 16) | (buf[9] << 8) | buf[8];
  if (totlen == 0) {
    printf("keylen invalid\n");
    valid = false;
  }

  // ignore opaque - 12-15
  // ignore cas - 16-23

  // formulate response
  buf[0] = 0x81;
  buf[7] = 0x0000;
  int ret = 0;

/*
   0x0000  No error
   0x0001  Key not found
   0x0002  Key exists  - we don't care- same as no error
   0x0003  Value too large
   0x0004  Invalid arguments
   0x0005  Item not stored
   0x0006  Incr/Decr on non-numeric value.
*/
  unsigned char* pErr = new unsigned char[33];
  *len_out = 33;
  memset(pErr, 0, 33);
  pErr[0] = 0x81;
  pErr[7] = 0x01;
  pErr[11] = 0x09;
  memcpy(pErr+24, "Not found",9);

  if (!valid) {
    printf("not valid input, returning error\n");
    return pErr;
  }

  // get
  if (buf[1] == 0x00) {

    // todo: get key - check offset for get
    std::string key;
    key.assign(buf[31], keylen);

    //0x0001  Key not found
    //0x0002  Key exists  - we don't care- same as no error

    std::string value = SimpleCache::getCreateInstance()->get(key);

    if (!value.length()) {
      printf("key not found %s\n",key.c_str());
      return pErr;
    } else {

      printf("got key %s len %d value %s len %d\n",key.c_str(), key.length(), value.c_str(), value.length());

      int len = 32 + value.length();

      delete pErr;
      pErr = new unsigned char[len];
      *len_out = len;
      memset(pErr, 0, len);
      pErr[0] = 0x81;
      pErr[4] = 0x04;
      pErr[7] = 0x01;
      pErr[11] = value.length() + 4; // this will break

      //boilerplate
      pErr[23] = 0x01;
      pErr[24] = 0xDE;
      pErr[25] = 0xAD;
      pErr[26] = 0xBE;
      pErr[27] = 0xEF;
      memcpy(pErr+28, value.c_str(), value.length());
      return pErr;
    }
  }

  // set 
  if (buf[1] == 0x01) {
    // todo: store key + value

    // ignore flags - 24-27
    // ignore expiry - 28-31

    // get key
    std::string key;
    key.assign(buf[31], keylen);

    // there is always flags and expiry
    unsigned int valuelen = totlen - keylen - 8;

    std::string value;
    value.assign(buf[31+keylen], valuelen);

    printf("storing key %s len %d value %s len %d\n",key.c_str(), keylen, value.c_str(), valuelen);

    SimpleCache::getCreateInstance()->put(key, value);

  }

  return pErr;
}

void thread_function(int new_fd)
{
  int spin = 0;
  while(1) {
    // local buffer
    unsigned char buffer[MAX_BUFFER];
    int bytesToRead = 0;
    int ret = 0;

    bytesToRead = recv(new_fd, buffer, MAX_BUFFER-1, 0);
    if (bytesToRead)
    {
      spin = 0;

      printf("server: got message len %d  %s\n", bytesToRead, buffer+24);

      unsigned char* bufOut = parse_and_process(buffer, &bytesToRead);

      printf("server: sending message len %d  %s\n", bytesToRead, bufOut+24);

      if (send(new_fd, bufOut, bytesToRead, 0) == -1) {
        perror("send");
      } else {
        printf("sent response %s\n",&buffer[24]);
      }
    } 
    else 
    {
      if (spin++ == 100) break;
      usleep(10);
    }
  }
  printf("thread is exiting\n");
  close(new_fd);

}

//
// borrowed from: 
// http://beej.us/guide/bgnet/examples/server.c
//
// good enough for osx basic socket impl
//
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (-1==bind(sockfd, p->ai_addr, p->ai_addrlen)) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;

        // this should block
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);

        printf("server: got connection from %s\n", s);

        // instead of forking here we spawn a thread and go to the next request
        //we could std::move(new_fd)- doesn't matter
        std::thread t(&thread_function, new_fd);
        t.detach();
    }

    return 0;
}

