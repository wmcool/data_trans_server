// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include "vector"
#include "utils.h"

#define DATA_PORT 8080
#define CONTROL_PORT 8081

#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

#define SHUT_SFD1 do {                                \
                            if (server_fd1 >= 0) {                 \
                                shutdown(server_fd1, SHUT_RDWR);   \
                                close(server_fd1);                 \
                                server_fd1 = -1;                   \
                            }                               \
                        } while (0)

#define SHUT_SFD2 do {                                \
                            if (server_fd2 >= 0) {                 \
                                shutdown(server_fd2, SHUT_RDWR);   \
                                close(server_fd2);                 \
                                server_fd2 = -1;                   \
                            }                               \
                        } while (0)

#define SHUT_FD1 do {                                \
                            if (fd1 >= 0) {                 \
                                shutdown(fd1, SHUT_RDWR);   \
                                close(fd1);                 \
                                fd1 = -1;                   \
                            }                               \
                        } while (0)

#define SHUT_FD2 do {                                \
                            if (fd2 >= 0) {                 \
                                shutdown(fd2, SHUT_RDWR);   \
                                close(fd2);                 \
                                fd2 = -1;                   \
                            }                               \
                        } while (0)

static int listen_socket(int listen_port, struct sockaddr_in& address)
{
    int server_fd, new_socket, valread;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(listen_port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

int main(int argc, char const* argv[])
{
    int server_fd1, server_fd2;
    struct sockaddr_in address1, address2;
    int addrlen1 = sizeof(address1);
    int addrlen2 = sizeof(address2);
    bool flag1 = false;
    bool flag2 = false;
    // 接收传感器数据
    server_fd1 = listen_socket(DATA_PORT, address1);
    // 接收平台指令
    server_fd2 = listen_socket(CONTROL_PORT, address2);
    int fd1 = 0;
    int fd2 = 0;
    if(server_fd1 == -1 || server_fd2 == -1) {
        exit(EXIT_FAILURE);
    }
    for(;;) {
        char buffer[158] = { 0 };
        int r;
        fd_set rd, wr, er;
        // 用位图来表示当前运行的算法，0表示没有算法在运行，1<<1 表示1号算法，1<<2表示2号算法
        int algo = 0;

        FD_ZERO(&rd);
        FD_ZERO(&wr);
        FD_ZERO(&er);
        FD_SET(server_fd1, &rd);
        FD_SET(server_fd2, &rd);
        if(flag1) FD_SET(fd1, &rd);
        if(flag2) FD_SET(fd2, &rd);
        // 获取nfds的值。并把fd1,fd2分别加入到，可读，可写，异常监视集合中去。
        r = select(max(server_fd1, max(server_fd2, max(fd1, fd2))) + 1, &rd, &wr, &er, NULL);
        if (r == -1 && errno == EINTR) {
            continue;
        }
        if (r == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        }
        if(FD_ISSET(server_fd1, &rd) && !flag1) {
            if ((fd1 = accept(server_fd1,
                                     (struct sockaddr *)&address1, (socklen_t*)&addrlen1))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            flag1 = true;
        }
        if(FD_ISSET(server_fd2, &rd) && !flag2) {
            if ((fd2 = accept(server_fd2,
                              (struct sockaddr *)&address2, (socklen_t*)&addrlen2))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            flag2 = true;
        }
        if(FD_ISSET(fd1, &rd)) {
            algo = 1;
            int varread = recv(fd1, buffer, 158, 0);
            if(varread <= 0 || algo == 0) continue;
            printf("%s\n", buffer);
            std::vector<double> data;
            data.push_back((double)covert2Int(buffer, 2, 10) / 10); // 土壤传感器数据
            data.push_back((double) covert2Int(buffer, 10, 18))
        }
        if(FD_ISSET(fd2, &rd)) {
            // todo 平台控制逻辑
            int varread = recv(fd2, buffer, 79, 0);
            if(varread <= 0) continue;
            if(strcmp(buffer, "exit") == 0) {
                SHUT_SFD1;
                SHUT_SFD2;
                SHUT_FD1;
                SHUT_FD2;
                break;
            }
            printf("%s\n", buffer);
        }
    }
}