// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "iostream"
#include <unistd.h>
#include <cerrno>
#include <sstream>
#include <sys/fcntl.h>
#include "vector"
#include "include/utils.h"
#include "include/mysocket.h"
#include "fstream"

#define DATA_PORT 8080
#define CONTROL_PORT 8081
#define NUM_ALGO 15

#undef max
#define max(x, y) ((x) > (y) ? (x) : (y))

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

#define SHUT_FD do {                                \
                            if (send_fd >= 0) {                 \
                                shutdown(send_fd, SHUT_RDWR);   \
                                close(send_fd);                 \
                                send_fd = -1;                   \
                            }                               \
                        } while (0)

#define SHUT_CFDS for(int i=0;i<max_clients;i++) { \
                        int sd = client_sockets[i];\
                        if(sd >= 0) {              \
                            shutdown(sd, SHUT_RDWR); \
                            close(sd);             \
                            client_sockets[i] = -1;                           \
                        }                          \
                        }                          \


static int listen_socket(int listen_port, struct sockaddr_in &address) {
    int server_fd;
    int opt = 1;

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
    if (bind(server_fd, (struct sockaddr *) &address,
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

int main(int argc, char const *argv[]) {
    int server_fd1, server_fd2;
    struct sockaddr_in address1, address2;
    int send_fd = init_socket();
    int addrlen1 = sizeof(address1);
    int addrlen2 = sizeof(address2);
    int pipe_fds[NUM_ALGO];
    for (int i = 0; i < NUM_ALGO; i++) {
        pipe_fds[i] = 0;
    }
//    std::fstream out("data.txt", std::ios::out);
    std::fstream in("data24.txt", std::ios::in);
    bool flag1 = false;
    bool flag2 = false;
    // 接收传感器数据
    server_fd1 = listen_socket(DATA_PORT, address1);
    // 接收平台指令
    server_fd2 = listen_socket(CONTROL_PORT, address2);
    int fd1 = 0;
    int fd2 = 0;
    if (server_fd1 == -1 || server_fd2 == -1) {
        exit(EXIT_FAILURE);
    }
    int fk_fd = open("data24.txt", O_RDONLY);

    for (;;) {
        std::string s;
        char control[40] = {0};
        bool algos[NUM_ALGO];
        for (int i = 0; i < NUM_ALGO; i++) {
            algos[i] = false;
        }
        int r = 0;
        fd_set rd, wr, er;

        FD_ZERO(&rd);
        FD_ZERO(&wr);
        FD_ZERO(&er);
        FD_SET(server_fd1, &rd);
        FD_SET(server_fd2, &rd);
        FD_SET(fk_fd, &rd);
        if (flag1)
            FD_SET(fd1, &rd);
        if (flag2)
            FD_SET(fd2, &rd);

        int max_sd = max(server_fd1, max(server_fd2, max(fd1, max(fd2, fk_fd))));
        r = select(max_sd + 1, &rd, &wr, &er, NULL);
        if (r == -1 && errno == EINTR) {
            continue;
        }
        if (r == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(server_fd1, &rd) && !flag1) {
            if ((fd1 = accept(server_fd1,
                              (struct sockaddr *) &address1, (socklen_t *) &addrlen1)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            flag1 = true;
        }
        if (FD_ISSET(server_fd2, &rd)) {
            int new_socket;
            if ((new_socket = accept(server_fd2,
                                     (struct sockaddr *) &address2, (socklen_t *) &addrlen2)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            fd2 = new_socket;
            flag2 = true;
        }
        if (FD_ISSET(fk_fd, &rd)) {
            getline(in, s);
            if(s.empty()) {
                std::cout << "end of file!" << std::endl;
                return 0;
            }
            std::stringstream ss;
            ss << s << "\n";
            s = ss.str();
            std::cout << s;
            for (int i = 0; i < NUM_ALGO; i++) {
                if (pipe_fds[i] != 0) {
                    int wtf = write(pipe_fds[i], s.c_str(), s.size());
                    if(wtf <= 0) {
                        printf("write pipe failed");
                        return 0;
                    }
                }
            }
            int ret = send(send_fd, s.c_str(), s.size(), 0);
            if(ret <= 0) {
                printf("send data failed");
                return 0;
            }
        }
        if (FD_ISSET(fd2, &rd)) {
            // 平台控制逻辑
            int varread = recv(fd2, control, 40, 0);
            if (varread <= 0) continue;
            if (strcmp(control, "FML-A") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/fmla", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    popen("./apia.sh", "r");
                    return 0;
                }
                int pipe_fd = open("/tmp/fmla", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }

            } else if (strcmp(control, "FML-B") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/fmlb", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    popen("./apib.sh", "r");
                    return 0;
                }
                int pipe_fd = open("/tmp/fmlb", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }
            } else if (strcmp(control, "FML-C") == 0) {

            } else if (strcmp(control, "INC-A") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/inca", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    popen("./inca.sh", "r");
                    return 0;
                }
                int pipe_fd = open("/tmp/inca", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }
            } else if (strcmp(control, "INC-B") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/incb", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    execl("incb/incb.sh", NULL);
                    return 0;
                }
                int pipe_fd = open("/tmp/incb", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }
            } else if (strcmp(control, "INC-C") == 0) {

            } else if (strcmp(control, "INC-D") == 0) {

            } else if (strcmp(control, "ND-A") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/nda", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    execl("ND-A/cpod", "-R", "4.0", "-W", "10", "-S", "2", "-K", "2", NULL);
                    return 0;
                }
                int pipe_fd = open("/tmp/nda", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }
            } else if (strcmp(control, "ND-B") == 0) {

            } else if (strcmp(control, "ND-C") == 0) {
                // 管道操作
                if ((mkfifo("/tmp/ndc", O_CREAT | O_EXCL) < 0) && (errno != EEXIST)) {
                    printf("cannot create fifo\n");
                    return 0;
                }
                if (errno == ENXIO) {
                    printf("open error; no reading process\n");
                    return 0;
                }
                int pid = fork();
                if (pid == 0) {
                    execl("ND-C/gau", NULL);
                    return 0;
                }
                int pipe_fd = open("/tmp/ndc", O_WRONLY, 0);
                if (pipe_fd <= 0) {
                    printf("open fifo failed");
                    break;
                }
                for (int i = 0; i < NUM_ALGO; i++) {
                    if (pipe_fds[i] == 0) {
                        pipe_fds[i] = pipe_fd;
                        break;
                    }
                }
            } else if (strcmp(control, "ND-D") == 0) {

            } else if (strcmp(control, "exit") == 0) {
                SHUT_SFD1;
                SHUT_SFD2;
                SHUT_FD1;
                SHUT_FD2;
//                    SHUT_CFDS;
//                    SHUT_FD;
                return 0;
            }
            printf("%s\n", control);
        }
//        sleep(1);
    }
}