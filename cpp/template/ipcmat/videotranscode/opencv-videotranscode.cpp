/**
  @file videocapture_basic.cpp
  @brief A very basic sample for using VideoCapture and VideoWriter
  @author PkLab.net
  @date Aug 24, 2016
*/
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/vpp.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <iostream>
#include <stdio.h>
#include <queue>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#endif

/************************************************************/
/* This is a stream socket server sample program for UNIX   */
/* domain sockets. This program listens for a connection    */
/* from a client program, accepts it, reads data from the   */
/* client, then sends data back to connected UNIX socket.   */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

// openCV headers

#define SOCK_PATH  "tpf_unix_sock.server"


int init_server(const char *sock_path, int backlog);
void print_usage(const char *prog_name);
int send_cvMat(int socket, cv::Mat &mat);
void sig_handler(int signo);


int main(int argc, char *argv[])
{
    int server_sock, client_sock, rc;
    socklen_t len;
    struct sockaddr_un client_sockaddr;     
    int backlog = 10;

    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    if (argc != 2) {
        print_usage(argv[0]);
        return -1;
    }

    // setup signal handler
    signal(SIGINT, sig_handler);
    printf("Enter ctrl+c to exit!!\n");
    cv::VideoCapture cap;
    if (!cap.open("a.264", cv::CAP_ANY, 0)) {
        std::cout << "open " << "aaaa" << " failed!" << std::endl;
        return NULL;
    }
    cap.set(cv::CAP_PROP_OUTPUT_YUV, PROP_TRUE);

    int height = (int) cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int width  = (int) cap.get(cv::CAP_PROP_FRAME_WIDTH);
    

    cv::Mat img;
    cap.read(img);
    cv::imwrite("send.jpg", img);



    // read an image
    //cv::Mat img_old = cv::imread(argv[1],cv::IMREAD_AVFRAME);
    //std::cout << img_old.rows << " " << img_old.cols << std::endl;
    //
    //cv::Mat img;
    //AVFrame *ff = cv::av::create(img_old.rows , img_old.cols , 0);
    //img.create(ff,0);
    //img_old.copyTo(img);
    //img.copyTo(img_old);
    std::cout << "size "<<img.rows << " " << img.cols << std::endl;

    // initilizing Server
    server_sock = init_server(SOCK_PATH, backlog);
    if (server_sock < 0) {
        printf("init server failed...\n");
        return -1;
    }

    printf("socket listening...\n");

    while (true) {
        /*********************************/
        /* Accept an incoming connection */
        /*********************************/
        printf("waiting for incoming connection...\n");
        client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
        if (client_sock == -1){
            perror("ACCEPT ERROR");
            close(server_sock);
            close(client_sock);
            exit(-1);
        }

        /****************************************/
        /* Get the name of the connected socket */
        /****************************************/
        len = sizeof(client_sockaddr);
        rc = getpeername(client_sock, (struct sockaddr *) &client_sockaddr, &len);
        if (rc == -1){
            perror("GETPEERNAME ERROR");
            close(server_sock);
            close(client_sock);
            exit(1);
        }
        else {
            printf("Client socket filepath: %s\n", client_sockaddr.sun_path);
        }


        /******************************************/
        /* Send cv Data to the connected socket */
        /******************************************/
        while(1){
            send_cvMat(client_sock, img);
            sleep(10);
        }

        /******************************/
        /* Close the sockets and exit */
        /******************************/
        close(client_sock);
    }
    cap.release();

    close(server_sock);

    return 0;
}

int init_server(const char *sock_path, int backlog)
{
    int sock = -1;
    int rc = -1;
    socklen_t len = 0;
    struct sockaddr_un server_sockaddr;

    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));

    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1){
        perror("SOCKET ERROR");
        return -1;
    }

    
    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to bind to.    */
    /*                                     */
    /* Unlink the file so the bind will    */
    /* succeed, then bind to that file.    */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;   
    strcpy(server_sockaddr.sun_path, sock_path); 
    len = sizeof(server_sockaddr);
    
    unlink(SOCK_PATH);
    rc = bind(sock, (struct sockaddr *) &server_sockaddr, len);
    if (rc == -1){
        perror("BIND ERROR");
        close(sock);
        exit(1);
    }

    /*********************************/
    /* Listen for any client sockets */
    /*********************************/
    rc = listen(sock, backlog);
    if (rc == -1){ 
        perror("LISTEN ERROR");
        close(sock);
        return -1;
    }

    return sock;
}

void print_usage(const char *prog_name)
{
    printf("Usage: %s file_name\n", prog_name);
}

void sig_handler(int signo)
{
    printf("signal number: %d caught!\n", signo);
    exit(EXIT_SUCCESS);
}


int send_cvMat(int socket, cv::Mat &mat)
{
    union ipc_mat{
        struct{
            unsigned long long addr;
            int height;
            int width;
            int total;
            int type;
            size_t step[2];
            int plane_size[4];
            int plane_step[4];
            int pix_fmt;
            int color_space;
            int color_range;
            int dev_id;
            int isYuvMat;
            int umat_size;
        };
        unsigned char data[256];
    } ipcMat;
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];
    struct iovec io = {&ipcMat.data, sizeof(ipcMat.data)};

    memset(buf, 0, sizeof(buf));
    memset(ipcMat.data, 0, sizeof(ipcMat.data));

    ipcMat.addr = mat.u->addr;
    ipcMat.height = mat.rows;
    ipcMat.width = mat.cols;
    ipcMat.isYuvMat = mat.avOK() ? 1 : 0;

    if (ipcMat.isYuvMat){  // 处理yuvMat
        ipcMat.plane_size[0] = mat.avAddr(5) - mat.avAddr(4); //avAddr(4~6)对应设备内存
        ipcMat.plane_step[0] = mat.avStep(4);

        ipcMat.pix_fmt = mat.avFormat();
        if (ipcMat.pix_fmt == AV_PIX_FMT_YUV420P){
            ipcMat.plane_size[2] =
            ipcMat.plane_size[1] = mat.avAddr(6) - mat.avAddr(5);
            ipcMat.plane_step[1] = mat.avStep(5);
            ipcMat.plane_step[2] = mat.avStep(6);
        } else if (ipcMat.pix_fmt == AV_PIX_FMT_NV12){
            ipcMat.plane_size[1] = ipcMat.plane_size[0] / 2;
            ipcMat.plane_step[1] = mat.avStep(5);
        }   // 此处仅供展示，更多的色彩格式可以继续扩展

        ipcMat.color_space = mat.u->frame->colorspace;
        ipcMat.color_range = mat.u->frame->color_range;
        ipcMat.dev_id = mat.card;
        ipcMat.total =  mat.total();
        ipcMat.umat_size = mat.u->size;
    } else { // 处理bgrMat
        ipcMat.total = mat.total();
        ipcMat.type = mat.type();
        ipcMat.step[0] = mat.step[0];
        ipcMat.step[1] = mat.step[1];
    }

    // print debug info
    printf("h:%d, w:%d, total:%d, type:%d, addr:0x%llx, fd:%d, step[0]:%lu, step[1]:%lu\n",
        ipcMat.height,
        ipcMat.width,
        ipcMat.total,
        ipcMat.type,
        ipcMat.addr,
        mat.u->fd,
        mat.step[0],
        mat.step[1]);

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *((int*)CMSG_DATA(cmsg)) = mat.u->fd;


    if (sendmsg (socket, &msg, 0) < 0) {
        perror ("Failed to send message");
        return -1;
    }

    return 0;
}

