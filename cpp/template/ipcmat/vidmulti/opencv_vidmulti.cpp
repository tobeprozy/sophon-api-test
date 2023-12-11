#include <opencv2/core.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/vpp.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <thread>
#include <random>
#include <stdlib.h>
#include <signal.h>
#include <queue>
#include <mutex>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <signal.h>
#include <conio.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <execinfo.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <iostream>

#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

// function declarations
void sig_handler(int signo);
int init_client(const char *client_path, const char *server_path);
int recv_cvMat(int socket, cv::Mat &mat);
void image_process(cv::Mat &mat);

int main(int argc, char *argv[])
{
    printf("clinet----\n");
    int client_sock;

    // setup signal handler
    signal(SIGINT, sig_handler);

    // initializing client
    client_sock = init_client(CLIENT_PATH, SERVER_PATH);
    if (client_sock < 0) {
        printf("init client failed...\n");
        return -1;
    }


    /**************************************/
    /* Read the data sent from the server */
    /* and process it.                      */
    /**************************************/
    int idx = 0;
    while(idx < 10){
        {
            cv::Mat mat;
            if(recv_cvMat(client_sock, mat) < 0) {
                close(client_sock);
                return -1;
            }

        // image processing
            image_process(mat);

        }
        sleep(11);
        idx++;
        if(idx == 5)
            break;
    }
    // pause
    {
        printf("input any key to exit:");
        getchar();
    }


    /******************************/
    /* Close the socket and exit. */
    /******************************/
    close(client_sock);
    
    return 0;
}

int init_client(const char *client_path, const char *server_path)
{
    int rc = -1;
    int sock = -1;
    socklen_t len = 0;
    struct sockaddr_un server_sockaddr; 
    struct sockaddr_un client_sockaddr; 

    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
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
    client_sockaddr.sun_family = AF_UNIX;   
    strcpy(client_sockaddr.sun_path, client_path); 
    len = sizeof(client_sockaddr);
    
    unlink(client_path);
    rc = bind(sock, (struct sockaddr *) &client_sockaddr, len);
    if (rc == -1){
        perror("BIND ERROR");
        close(sock);
        exit(1);
    }
        
    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* for the server socket and connect   */
    /* to it.                              */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, server_path);
    rc = connect(sock, (struct sockaddr *) &server_sockaddr, len);
    if(rc < -1){
        perror("CONNECT ERROR");
        close(sock);
        return -1;
    }

    return sock;
}

void sig_handler(int signo)
{
    printf("signal number: %d caught!\n", signo);
    exit(EXIT_SUCCESS);
}

int recv_cvMat(int socket, cv::Mat &mat)
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
    int fd = 0;

    memset(buf, 0, sizeof(buf));
    memset(ipcMat.data, 0, sizeof(ipcMat.data));

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg (socket, &msg, 0) < 0) {
        perror("Failed to receive message");
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    fd = *(int *) CMSG_DATA(cmsg);
    //void *va = mmap(NULL, ipcMat.umat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
    //if (va == NULL) {
    //    perror("MMAP ERROR");
    //    return -1;
    //}

    printf("h:%d, w:%d, total:0x%x, type:%d, paddr:0x%llx, vaddr:0x%llx, fd:%d, step[0]:%lu, step[1]:%lu, isYUV:%d\n",
        ipcMat.height,
        ipcMat.width,
        ipcMat.total,
        ipcMat.type,
        ipcMat.addr,
        0,
        fd,
        ipcMat.step[0],
        ipcMat.step[1],
        ipcMat.isYuvMat);


    if(ipcMat.isYuvMat) { // yuvMat
        AVFrame *f = cv::av::create(ipcMat.height,
                                    ipcMat.width,
                                    ipcMat.pix_fmt,
                                    0,
                                    ipcMat.addr,
                                    fd,
                                    ipcMat.plane_step,
                                    ipcMat.plane_size,
                                    ipcMat.color_space,
                                    ipcMat.color_range,
                                    ipcMat.dev_id);
        mat.create(f, ipcMat.dev_id);

        struct cv::UMatOpaque * mat_opaque = (cv::UMatOpaque *)f->opaque;
        if (mat_opaque && mat_opaque->magic_number == MAGIC_MAT){
            cv::UMatData *v = mat_opaque->data;

            v->size = ipcMat.umat_size;
            v->flags &= ~cv::UMatData::USER_ALLOCATED;
            v->flags &= ~cv::UMatData::DEVICE_MEM_ATTACHED;
        }
    } else {
        mat.create(ipcMat.height,
                   ipcMat.width,
                   ipcMat.total,
                   ipcMat.type,
                   ipcMat.step,
                   0,
                   ipcMat.addr,
                   fd,
                   ipcMat.dev_id);
        mat.u->flags &= ~cv::UMatData::USER_ALLOCATED;
        mat.u->flags &= ~cv::UMatData::DEVICE_MEM_ATTACHED;
    }

    if (mat.empty()) {
        return -1;
    }

    return 0;
}

void image_process(cv::Mat &mat)
{
   // ================debug start===================
    printf("h:%d, w:%d, total:%lu, type:%d, addr:0x%llx, fd:%d, step[0]:%lu, step[1]:%lu, isYUV:%d\n",
        mat.rows,
        mat.cols,
        mat.total(),
        mat.type(),
        mat.u->addr,
        mat.u->fd,
        mat.step[0],
        mat.step[1],
        mat.avOK() ? 1 : 0);
    printf("FUNC:LINE(%s:%d)\n", __FUNCTION__, __LINE__);
    // ================debug end===================

    // processing code HERE!!
    //cv::bmcv::resize(mat, mat, cv::Size(512,512));

    //cv::Mat img;
    //AVFrame *ff = cv::av::create(mat.rows , mat.cols , 0);
    //img.create(ff,0);
    //mat.copyTo(img);
    //close(mat.u->fd);
    //std::cout << "resize "<<mat.rows << " " << mat.cols << std::endl;
    //printf("umat------------flags = %d\n",mat.u->flags);
    //munmap((void *)mat.u->addr,mat.total());

    cv::imwrite("out.jpg", mat);
}

