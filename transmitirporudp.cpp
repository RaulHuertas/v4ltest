#include "transmitirporudp.hpp"
#include <unistd.h>
#include <string.h>

using namespace std;

void transmitirPorUDP(
    int ancho, int alto, int formatoStream,
    const std::vector<unsigned char>& datos,
    int sockfd, struct sockaddr_in& serveraddr, int serverlen
){
    transmitirPorUDP(
        ancho, alto, formatoStream,
        datos.data(), datos.size(),
        sockfd, serveraddr, serverlen
    );
}

void transmitirPorUDP(
    int ancho, int alto, int formatoStream,
    const unsigned char* datosATransmitir, int longDatos,
    int sockfd, struct sockaddr_in& serveraddr, int serverlen
){
    constexpr int sizePaquetes = 2*2048;
    constexpr int headerSize = 1+5*sizeof(int);
    int nPacks = longDatos/sizePaquetes;
    int bytesSobrantes = longDatos%sizePaquetes;
    if(bytesSobrantes>0){
        nPacks++;
    }
    int offsetDatos =0;
    std::vector<unsigned char> bufferTrans;
    bufferTrans.resize(headerSize+sizePaquetes);
    int porEnviar = longDatos;
    const int totalImageSize=longDatos;
    for(int p = 0; p<nPacks; p++){
        unsigned char *datos = bufferTrans.data();
        datos[0] = 0xFF; datos+=1;
        //cout<<"EnviandoOFFSET: "<<offsetDatos<<endl;
        memcpy(datos, &offsetDatos, sizeof(offsetDatos)); datos+=sizeof(offsetDatos);
        memcpy(datos, &totalImageSize, sizeof(totalImageSize)); datos+=sizeof(totalImageSize);
        memcpy(datos, &ancho, sizeof(ancho)); datos+=sizeof(ancho);
        memcpy(datos, &alto, sizeof(alto)); datos+=sizeof(alto);
        memcpy(datos, &formatoStream, sizeof(formatoStream)); datos+=sizeof(formatoStream);
        int nDatosAEnviarEnEstePaquete = std::min(sizePaquetes, porEnviar);
        memcpy(
               datos,
               &datosATransmitir[offsetDatos],
               nDatosAEnviarEnEstePaquete
               );datos+=nDatosAEnviarEnEstePaquete;
        const int packSize=nDatosAEnviarEnEstePaquete+headerSize;
        sendto(
               sockfd,
               bufferTrans.data(),
               packSize,
               0,
               (sockaddr*)&serveraddr,
               serverlen
               );

        porEnviar-=nDatosAEnviarEnEstePaquete;
        offsetDatos+=nDatosAEnviarEnEstePaquete;
    }
}

bool Conexion::abrirConexion(){
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        return false;
    }
    server = gethostbyname(hostname);
    if(server == NULL) {
        return false;
    }
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    serverlen = sizeof(serveraddr);

    return true;
}
