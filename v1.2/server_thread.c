#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sockInfo {
    int fd;         //通信的文件描述符
    pthread_t tid;  // 线程号
    struct sockaddr_in addr;
};

// 支持128个客户端连接
struct sockInfo infos[128];

// 子线程与客户端通信
void* working(void* arg) {
    struct sockInfo* pinfo_client = (struct sockInfo*)arg;

    // 获取客户端信息
    char clientIp[16];
    inet_ntop(AF_INET, &pinfo_client->addr.sin_addr.s_addr, clientIp, sizeof(clientIp));
    unsigned short clientPort = ntohs(pinfo_client->addr.sin_port);
    printf("client ip is %s, port is %d\n", clientIp, clientPort);

    // 循环读取客户端发送的信息,并发送信息
    char recvBuf[1024] = {0};
    while (1) {
        int len = read(pinfo_client->fd, recvBuf, sizeof(recvBuf));
        if (len == -1) {
            perror("read");
            break; //客户端结束时服务器才不会停止,不能用exit();
        } else if (len > 0) {
            printf("recv client data: %s\n", recvBuf);
        } else if (len == 0) {
            printf("client closed...\n");
            break;
        }

        // 给客户端发送数据
        // char* data = "hello, i am server\n";
        char* data = recvBuf;
        int ret = write(pinfo_client->fd, data, strlen(data));
        if (ret == -1) {
            perror("write");
            break;
        }
    }
    // 关闭接收用的文件描述符
    close(pinfo_client->fd);
    // 后序使用
    pinfo_client->fd = -1;

    return NULL;
}

int main() {
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

    // 统一初始化线程用的结构体信息
    int max = sizeof(infos) / sizeof(infos[0]);
    for (int i = 0; i < max; ++i) {
        bzero(&infos[i], sizeof(infos[i]));
        infos[i].fd = -1;
        infos[i].tid = -1;
    }

    // 循环等待接收客户端连接
    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t size = sizeof(clientaddr);

        // 接受连接
        int cfd = accept(lfd, (struct sockaddr*)&clientaddr, &size);
        if (cfd == -1) {
            perror("accept");
            exit(-1);
        }

        // 需要传给子线程的信息
        struct sockInfo* pinfo;
        for (int i = 0; i < max; ++i) {
            // 找到一个可用结构体来通信
            if (infos[i].fd == -1) {
                pinfo = &infos[i];
                break;
            }
            if (i == max - 1) {
                sleep(1);
                i = -1;
            }
        }

        // 每一个连接进来，创建一个子线程跟客户端通信
        pinfo->fd = cfd;
        memcpy(&pinfo->addr, &clientaddr, size);
        pthread_create(&pinfo->tid, NULL, working, pinfo);

        // 回收子线程
        pthread_detach(pinfo->tid);
    }

    // 关闭监听文件描述符
    close(lfd);

    return 0;
}