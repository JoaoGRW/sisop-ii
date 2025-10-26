#include <iostream>
#include <ctime>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "messages.h"

#define TIMEOUT_SEC 2

uint32_t curr_balance;
uint32_t curr_seq_n = 0;

// Informações guardadas da última transação realizada pelo cliente
uint32_t transfer_dest_ip;
uint32_t transfer_amount;

// Função auxiliar para abrir o socket do cliente, configurado para broadcast
bool open_client_socket(int& sock, int port, struct sockaddr_in& local_addr){
    // Abrindo socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::cerr << "ERROR on opening\n";
        return false;
    }

    // Configurando socket para broadcast
    int broadcast_enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0){
        std::cerr << "ERROR on setsockopt SO_BROADCAST\n";
        return false;
    }

    // Configurando timeout da resposta do servidor
    struct timeval timeout;
    timeout.tv_sec  = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
        std::cerr << "ERROR on setsockopt SO_RCVTIMEO\n";
        return false;
    }
 
    // Fazendo o bind do socket
    local_addr.sin_family           = AF_INET;
    local_addr.sin_port             = htons(port);
    local_addr.sin_addr.s_addr      = INADDR_ANY;

    /*if (bind(sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr)) < 0){
        std::cerr << "ERROR on bind\n";
        return false;
    }*/

    return true;
}

// Funcao auxiliar abre a socket cliente
bool send_discovery_message(int& sock, int port, struct sockaddr_in& server_addr){  
    // Preparando o endereço de Broadcast
    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family       = AF_INET;
    broadcast_addr.sin_port         = htons(port);
    broadcast_addr.sin_addr.s_addr  = inet_addr("255.255.255.255");

    // Criando pacote de descoberta
    packet discovery_p;
    discovery_p.type = static_cast<uint16_t>(messageType::DISCOVERY);
    discovery_p.seq_n = 0;

    char buffer[256];
    socklen_t server_len = sizeof(server_addr);
    int n;

    
    while (true){
        sendto(sock, &discovery_p, sizeof(discovery_p), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        
        n = recvfrom(sock, buffer, 256, 0, (struct sockaddr*)&server_addr, &server_len);

        // Checa se o timeout foi estourado ou foi recebida uma resposta
        if (n < 0){
            std::cout << "WARNING discovery message timeout, resending...\n";
            continue;
        }

        // Processa resposta
        packet response_p = *(packet*)buffer;
        // Se a resposta foi um ACK de um pacote de descoberta atualiza o saldo e retorna
        if (response_p.type == static_cast<uint16_t>(messageType::DISCOVERY_ACK)){
            curr_balance = response_p.disc_ack.client_balance;
            return true;
        }
    }

    return false;
}

// Função auxiliar que imprime YYYY-MM-DD hh:mm:ss
void print_current_date_time(){
    time_t now = time(0);
    struct tm *dt = localtime(&now);

    std::cout << dt->tm_year + 1900 << '-' << dt->tm_mon << '-' << dt->tm_mday << ' '
    << dt->tm_hour << ':' << dt->tm_min << ':' << dt->tm_sec;
}

int main(int argc, char** argv){
    // Recebe a porta na qual o cliente irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Porta não informada.";
        return 1;
    }
    int port = atoi(argv[1]);
    int sock;
    uint32_t transfer_target_ip;
    int transfer_value;
    struct sockaddr_in local_addr, server_addr;

    // Abertura do socket do cliente com opção de broadcast
    if (!open_client_socket(sock, port, local_addr))
        return 1;

    // Envio da mensagem de descoberta do servidor, endereço será escrito em server_addr
    if (!send_discovery_message(sock, port, server_addr))
        return 1;
    print_current_date_time();
    std::cout << " server_addr " << server_addr.sin_addr.s_addr << '\n';


    // Lê da entrada padrão a próxima ação a ser feita
    while(true){
        std::cin >> transfer_dest_ip >> transfer_amount;

        // Checa se transfer_amount informado foi 0 (mostra saldo atual)
        if (transfer_amount == 0){
            std::cout << "curr_balance " << curr_balance << '\n';
            continue;
        }
        
        
    }

    return 0;
}