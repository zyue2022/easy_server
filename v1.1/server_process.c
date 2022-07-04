#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>

void recyleChild(int arg) {
    while (1) {
        // -1 参数表示想回收所有子进程
        int ret = waitpid(-1, NULL, WNOHANG);
        if (ret == -1) {
            // 所有子进程都已经回收
            break;
        } else if (ret == 0) {
            // 还有子进程存在
            break;
        } else if (ret > 0) {
            // 回收到子进程
            printf("子进程 %d 被回收...\n", ret);
        }
    }
}


int main() {
    // 注册信号捕捉，用于回收子进程
    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = recyleChild;
    sigaction(SIGCHLD, &act, NULL);

    // 创建用于监听的套接字
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket");
        exit(-1);
    }

    // 绑定本地地址和端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
    int ret = bind(lfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    ret = listen(lfd, 128);
    if (ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 循环等待接收客户端连接
    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t size = sizeof(clientaddr);

        // 接受连接
        int cfd = accept(lfd, (struct sockaddr*)&clientaddr, &size);
        if (cfd == -1) {
            // 如果某个已连接客户端退出，就让父进程结束；
            // 会使后序无法再次连接，所以需要判断错误号
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            exit(-1);
        }

        // 每一个连接进来，创建一个子进程跟客户端通信
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(-1);
        } else if (pid == 0) {
            // 子进程
            // 获取客户端信息
            char clientIp[16];
            inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIp, sizeof(clientIp));
            unsigned short clientPort = ntohs(clientaddr.sin_port);
            printf("client ip is %s, port is %d\n", clientIp, clientPort);
            // 循环读取客户端发送的信息,并发送信息
            char recvBuf[1024] = {0};
            while (1) {
                int len = read(cfd, recvBuf, sizeof(recvBuf));
                if (len == -1) {
                    perror("read");
                    exit(-1);
                } else if (len > 0) {
                    printf("recv client data: %s\n", recvBuf);
                } else if (len == 0) {
                    printf("client closed...\n");
                    break;
                }

                // 给客户端发送数据
                // char* data = "hello, i am server\n";
                char* data = recvBuf;
                write(cfd, data, strlen(data));
            }
            // 关闭接收用的文件描述符
            close(cfd);
            // 子进程结束
            exit(0);
        } 
    }

    // 关闭监听文件描述符
    close(lfd);

    return 0;
}