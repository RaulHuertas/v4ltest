#ifndef TRANSMITIRPORUDP_HPP
#define TRANSMITIRPORUDP_HPP
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void transmitirPorUDP(
    int ancho, int alto, int formatoStream,
    const std::vector<unsigned char>& datos,
    int sockfd, struct sockaddr_in& serveraddr, int serverlen
);

void transmitirPorUDP(
    int ancho, int alto, int formatoStream,
    const void* datos, int longDatos,
    int sockfd, struct sockaddr_in& serveraddr, int serverlen
);

struct Conexion{
    char* hostname;
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent* server;
    int serverlen;
    bool abrirConexion();
};



#endif // TRANSMITIRPORUDP_HPP

