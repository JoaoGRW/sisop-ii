#include <iostream>
#include <ctime>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "messages.h"

#define TIMEOUT_SEC 2

// Flags para impressao de informacoes na tela
bool show_curr_balance  = false;
bool show_last_transfer = false;
bool no_server_answer   = false;

// Informações guardadas da última transação realizada pelo cliente
std::mutex mtx;
std::condition_variable cv;

uint32_t transfer_dest_ip;
uint32_t transfer_amount;
uint32_t curr_balance;
uint32_t curr_seq_n = 0;

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

// Função auxiliar para mandar uma requisição de transferência
bool send_transfer_request(uint32_t dest_ip, int amount, int &new_balance, int sock, struct sockaddr_in server_addr){
    // Montando e enviando o pacote de requisicao
    packet transfer_req;
    curr_seq_n++;
    transfer_req.seq_n = curr_seq_n;
    transfer_req.type = static_cast<uint16_t>(messageType::REQUEST);
    // Usar 'amount' (parâmetro) e dest_ip (parâmetro)
    transfer_req.req = { dest_ip, static_cast<uint32_t>(amount) };

    char buffer[256];
    socklen_t len = sizeof(server_addr);
    int resend = 0;
    while (resend < 3){
        sendto(sock, &transfer_req, sizeof(transfer_req), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        int n = recvfrom(sock, &buffer, 256, 0, (struct sockaddr*)&server_addr, &len);
        if (n < 0) {
            // timeout ou erro no recvfrom
            resend++;
            continue;
        }
        packet p = *(packet*)buffer;
        if (p.type == static_cast<uint16_t>(messageType::REQUEST_ACK) && p.req_ack.seq_n == curr_seq_n){
            new_balance = p.req_ack.new_balance;
            return true;
        }
        resend++;
    }

    return false;
}

// Função que será executada pela thread responsável por mostrar informações na tela
void handle_output(){
    while (true){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return show_last_transfer || show_curr_balance || no_server_answer; });

        bool transfer = show_last_transfer;
        bool balance  = show_curr_balance;

        show_last_transfer = false;
        show_curr_balance  = false;

        // Verifica se deve imprimir informações da última transferência ou o saldo atual
        print_current_date_time();
        if (transfer){
            char buf[256];
            std::cout << " id_req " << curr_seq_n << " dest "
            << inet_ntop(AF_INET, &transfer_dest_ip, buf, sizeof(buf))
            << " value " << transfer_amount << " new_balance " << curr_balance << '\n';
        }
        if (balance){
            std::cout << "curr_balance " << curr_balance << '\n';
        }
    }
}

int main(int argc, char** argv){
    // Recebe a porta na qual o cliente irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Porta não informada.";
        return 1;
    }
    int port = atoi(argv[1]);
    int sock;
    struct sockaddr_in local_addr, server_addr;

    // Abertura do socket do cliente com opção de broadcast
    if (!open_client_socket(sock, port, local_addr))
        return 1;

    // Envio da mensagem de descoberta do servidor, endereço será escrito em server_addr
    if (!send_discovery_message(sock, port, server_addr))
        return 1;


    // Imprime as mensagens iniciais de abertura do cliente
    char buf[INET_ADDRSTRLEN + 1];  
    print_current_date_time();
    std::cout << " server_addr " << inet_ntop(AF_INET, &server_addr.sin_addr, buf, sizeof(buf))
    << " curr_balance " << curr_balance << '\n';

    // Cria a thread que será responsável pela saída no terminal
    std::thread output_thread(handle_output);
    output_thread.detach();

    uint32_t dest_ip;
    int amount;
    std::string ip_string;

    // Thread principal lê a entrada padrão para requisições do usuário
    while(true){
        std::cin >> ip_string >> amount;

        // Se o usuário só quer ver o saldo atual (amount == 0)
        if (amount == 0){
            show_curr_balance = true;
            // notificar a thread de output
            cv.notify_one();
            continue;
        }

        // converte string "x.x.x.x" para uint32_t (network order)
        struct in_addr inaddr;
        if (inet_aton(ip_string.c_str(), &inaddr) == 0) {
            std::cerr << "IP inválido: " << ip_string << '\n';
            continue;
        }
        dest_ip = inaddr.s_addr; // já em network byte order

        int new_balance;
        if (send_transfer_request(dest_ip, amount, new_balance, sock, server_addr)){
            // Modifica as variáveis globais que guardam as informações da última transferência
            {
                std::lock_guard<std::mutex> lock(mtx);
                transfer_dest_ip    = dest_ip;
                transfer_amount     = amount;
                curr_balance        = new_balance;
                show_last_transfer  = true;
            }
            // Notifica a thread de interface que houve uma nova transação
            cv.notify_one();
        } else {
            no_server_answer = true;
            cv.notify_one();
        }
    }

    return 0;
}