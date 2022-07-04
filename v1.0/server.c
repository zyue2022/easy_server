#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    // 创建用于监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket");
        exit(-1);
    }

    // 绑定本地地址和端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    // inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(9999);
    int ret = bind(lfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    ret = listen(lfd, 8);
    if (ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 接收客户端连接，阻塞
    struct sockaddr_in clientaddr;
    socklen_t length = sizeof(clientaddr);
    int cfd = accept(lfd, (struct sockaddr*)&clientaddr, &length);
    if (cfd == -1) {
        perror("accept");
        exit(-1);
    }

    // 输出客户端信息
    char clientIp[16];
    inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIp, sizeof(clientIp));
    unsigned short clientPort = ntohs(clientaddr.sin_port);
    printf("client ip is %s, port is %d\n", clientIp, clientPort);

    char recvBuf[1024] = {0};
    while (1) {
        // 读取客户端发送的信息
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
        char* data = "hello, i am server\n";
        write(cfd, data, strlen(data));
    }

    // 关闭文件描述符
    close(lfd);
    close(cfd);

    return 0;
}