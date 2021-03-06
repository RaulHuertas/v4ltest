#include <sys/time.h>
#include <iostream>
#include <vector>
// api udp
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "transmitirporudp.hpp"
#include "opencv2/opencv.hpp"

static volatile sig_atomic_t terminar = 0;
void signal_handler(int signal)
{
    terminar = 1;
}

unsigned long long tiempoActual_ms(){
    timeval time;
    gettimeofday(&time, NULL);
    unsigned long long  millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    return millis;
}


using namespace std;
int main(int argc, char *argv[])
{
    char* hostname;
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent* server;
    int serverlen;


    if(argc!=5){
        printf("Argumentos del programa invalidos\n");
        printf("Invocarlo como: Test2 <direccionDestino>  <puertoDestino> <ancho> <alto>\n");
        exit(EXIT_FAILURE);
    }
    //Capturar señal de Ctrl+C para terminar e lprograma
    signal(SIGINT, signal_handler);

    //Captura de parametros de la linea de comandos
    int anchoImagen, altoImagen;
    hostname = argv[1];
    portno = atoi(argv[2]);
    anchoImagen = atoi(argv[3]);
    altoImagen = atoi(argv[4]);

    Conexion conn;
    conn.hostname = hostname;
    conn.portno = portno;
    if(!conn.abrirConexion()){
        printf("Error conexion \r\n");
        exit(EXIT_FAILURE);
    }
    //abrir el archivo de la camara
    int fd =0;
    if((fd=open("/dev/video0", O_RDWR))<0){
        printf("Error abriendo la camara");
    }
    //ver si es un dispositivo capaz de ser accesibles
    struct v4l2_capability cap;
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0){
        perror("VIDIOC_QUERYCAP");
        exit(1);
    }
    printf("Driver de la camara: %s\r\n", cap.driver);
    printf("Tarjeta de la camara: %s\r\n", cap.card);
    printf("Info del bus: %s\r\n", cap.bus_info);
    printf("Version: %d\r\n", cap.version);
    printf("Capacidades: %u\r\n", cap.capabilities);
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        fprintf(stderr, "Este dispositivo no puede capturar videos.\n");
        exit(1);
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
        fprintf(stderr, "Este dispositivo no puede capturar videos.\n");
        exit(1);
    }
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = anchoImagen;
    format.fmt.pix.height = altoImagen;
    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0){
        perror("VIDIOC_S_FMT");
        exit(1);
    }

    constexpr int nBufferesAPedir =8;
    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = nBufferesAPedir;

    if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
        perror("VIDIOC_REQBUFS");
        exit(1);
    }
    auto tipo = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    struct v4l2_buffer infoBufferes[nBufferesAPedir];
    void* buffer_start[nBufferesAPedir];
    for(int buff=0; buff<nBufferesAPedir; buff++){
        auto& bufferinfo = infoBufferes[buff];
        memset(&bufferinfo, 0, sizeof(bufferinfo));

        bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferinfo.memory = V4L2_MEMORY_MMAP;
        bufferinfo.index = buff;

        if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0){
            perror("VIDIOC_QUERYBUF");
            exit(1);
        }

        buffer_start[buff] = mmap(
            NULL,
            bufferinfo.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            bufferinfo.m.offset
        );

        if(buffer_start[buff] == MAP_FAILED){
            perror("mmap");
            exit(1);
        }

        memset(buffer_start[buff], 0, bufferinfo.length);

        //hacer e lbuffer disponible
        if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0){
            perror("VIDIOC_QBUF");
            exit(1);
        }
    }



    //activar streaming
    int type = tipo;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("VIDIOC_STREAMON");
        exit(1);
    }



    //bucle principal
    unsigned long long tiempoInicio = tiempoActual_ms();
    int cuardrosProcesados = 0;
    int sgteBufferALeer = 0;
    int nFrames = 0;
    char bufferNombreArchivo[50];

    if(ioctl(fd, VIDIOC_DQBUF, &infoBufferes[0]) < 0){
        perror("VIDIOC_QBUF");
        exit(1);
    }
    int bufferAnterior = sgteBufferALeer;
    sgteBufferALeer++;

    while(terminar==0){
        //esperar por el buffer de salida



        //PROCESAR LA INFO
        std::thread hilooProcesamiento(
        [&](){
            transmitirPorUDP(
                anchoImagen, altoImagen, 0,
                buffer_start[bufferAnterior], infoBufferes[bufferAnterior].length,
                conn.sockfd, conn.serveraddr, conn.serverlen
            );
            if(ioctl(fd, VIDIOC_QBUF, &infoBufferes[bufferAnterior]) < 0){
                   perror("VIDIOC_QBUF");
                   exit(1);
            }

            infoBufferes[bufferAnterior].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            infoBufferes[bufferAnterior].memory = V4L2_MEMORY_MMAP;

             unsigned long long tiempoActual =  tiempoActual_ms();
             cuardrosProcesados++;
             if((tiempoActual-tiempoInicio)>1000){
                printf("Cuadros procesados el ultimo segundo: %d\r\n", cuardrosProcesados);
                tiempoInicio = tiempoActual;
                cuardrosProcesados = 0;
                nFrames++;

             }
        }
        );

        auto& bufferinfo= infoBufferes[sgteBufferALeer];
        if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0){
            perror("VIDIOC_QBUF");
            exit(1);
        }
        hilooProcesamiento.join();
        bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferinfo.memory = V4L2_MEMORY_MMAP;
        bufferAnterior = sgteBufferALeer;


         sgteBufferALeer++;
         sgteBufferALeer%=nBufferesAPedir;
    }








    //desactivar streaming
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
        perror("VIDIOC_STREAMOFF");
        exit(1);
    }



    close(fd);
    return EXIT_SUCCESS;
}

