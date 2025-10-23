#include <iostream>
#include <ctime>
#include <string>
#include <cstring> // Para memset

// Headers de Rede
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // Para close()
#include <sys/time.h> // Para timeval (timeout)

#include "messages.h"

// Função para imprimir a descoberta do servidor no formato EXATO
//  Formato: YYYY-MM-DD HH:MM:SS server_addr [IP]
void print_server_discovery(struct sockaddr_in& server_addr) {
    char buffer[80];
    time_t now = time(0);
    struct tm * timeinfo = localtime(&now);
    // Formato: 2024-10-01 18:37:00
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

    std::cout << buffer << " server_addr " << ip_str << std::endl;
}


int main(int argc, char** argv){
    int port;

    if (argc < 2){
        std::cout << "Porta nao informada. Uso: ./cliente <porta>" << std::endl;
        return 1;
    }
    port = atoi(argv[1]);

    // --- Configuração do Socket Cliente ---
    int sockfd;
    struct sockaddr_in broadcast_addr, server_addr;
    socklen_t server_len = sizeof(server_addr);
    char buffer[sizeof(packet)];

    // 1. Criar Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERRO: Nao foi possivel abrir o socket." << std::endl;
        return 1;
    }

    // 2. Ativar Permissão de Broadcast 
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        std::cerr << "ERRO: Nao foi possivel ativar o broadcast." << std::endl;
        close(sockfd);
        return 1;
    }

    // 3. Configurar Endereço de Broadcast
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(port);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST; // Endereço de broadcast

    // 4. Preparar Pacote de Descoberta
    packet disc_packet;
    disc_packet.type = DESC;
    disc_packet.seq_n = 0;

    // 5. Configurar Timeout para Retransmissão [cite: 41, 69]
    // Usando 10ms como sugerido, mas pode ser muito curto. Vamos usar 1 segundo.
    struct timeval tv;
    tv.tv_sec = 1; // 1 segundo
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "ERRO: Nao foi possivel definir o timeout." << std::endl;
    }


    // --- Loop de Descoberta (enviar até receber resposta) ---
    std::cout << "Procurando servidor na porta " << port << "..." << std::endl;
    while (true) {
        // 6. Enviar Pacote de Descoberta [cite: 29]
        if (sendto(sockfd, &disc_packet, sizeof(disc_packet), 0,
                   (struct sockaddr *) &broadcast_addr, sizeof(broadcast_addr)) < 0) {
            std::cerr << "ERRO: sendto de descoberta." << std::endl;
            sleep(1);
            continue;
        }

        // 7. Esperar Resposta (DESC_ACK)
        int n = recvfrom(sockfd, buffer, sizeof(packet), 0,
                         (struct sockaddr *) &server_addr, &server_len);

        if (n < 0) {
            // Timeout 
            std::cout << "Timeout. Reenviando pacote de descoberta..." << std::endl;
        } else {
            // Pacote recebido
            packet* p = (packet*) buffer;
            if (p->type == DESC_ACK) {
                // Servidor encontrado! 
                print_server_discovery(server_addr);
                break; // Sai do loop de descoberta
            }
        }
    }

    // --- Fase de Processamento ---
    std::cout << "Conectado. Digite seu comando no formato: IP_DESTINO VALOR" << std::endl;
    
    // TODO: Desativar o timeout ou ajustá-lo para reenvio de REQ
    
    // TODO: Implementar as duas threads do cliente 
    // Thread 1: Ler comandos do stdin (IP_DESTINO VALOR)
    // Thread 2: Escrever mensagens na tela (respostas do servidor)

    // Por enquanto, um loop simples de leitura (a ser movido para uma thread)
    // std::string dest_ip;
    // int valor;
    // while (std::cin >> dest_ip >> valor) {
    //    // TODO: [cite: 138]
    //    // 1. Preparar pacote REQ
    //    // 2. Enviar com sendto() para o 'server_addr' salvo
    //    // 3. Esperar pelo REQ_ACK (com lógica de retransmissão)
    //    // 4. Imprimir resposta [cite: 144]
    // }


    close(sockfd);
    return 0;
}