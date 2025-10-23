#include <iostream>
#include <thread>
#include <ctime>
#include <unordered_map>
#include <list>
#include <string>
#include <cstring> // Para memset

// Headers de Rede
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // Para close()

#include "messages.h"

// Saldo inicial para novos clientes [cite: 30]
#define INITIAL_BALANCE 100

class Client{
    uint32_t ip_addr;
    int last_req;
    int balance;

    public:
    // Construtor Padrão (necessário para o map)
    Client() : ip_addr(0), last_req(0), balance(0) {}

    Client(uint32_t ip, int bal){
        this->ip_addr = ip;
        this->balance = bal;
        this->last_req = 0; // [cite: 77]
    }

    // Metodos getters
    uint32_t getIPAddress(){ return this->ip_addr; }
    int getLastRequestID(){ return this->last_req; }
    int getBalance(){ return this->balance; }

    void updateBalance(int amount_transfered){
        this->balance += amount_transfered;
    }

    void updateRequest(){
        this->last_req++;
    }
};

class Transaction{
    uint32_t origin_ip;
    int req_id;
    uint32_t dest_ip;
    int amount;

public:
    Transaction(uint32_t origin_ip, int req_id, uint32_t dest_ip, int amount){
        this->origin_ip = origin_ip;
        this->req_id    = req_id;
        this->dest_ip   = dest_ip;
        this->amount    = amount;
    }

    // Metodos getters
    uint32_t getOriginIP(){ return this->origin_ip; }
    int getReqID(){ return this->req_id; }
    uint32_t getDestIP(){ return this->dest_ip; }
    int getAmount(){ return this->amount; }
};

// Função auxiliar para imprimir as informações na tela no formato EXATO
// [cite: 114] Formato: YYYY-MM-DD HH:MM:SS
void print_info(int num_transactions, int total_transfered, int total_balance){
    char buffer[80];
    time_t now = time(0);
    struct tm * timeinfo = localtime(&now);
    // Formato: 2025-09-11 18:37:00
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    std::cout << buffer << " num_transactions " << num_transactions << " total_transfered "
    << total_transfered << " total_balance " << total_balance << std::endl;
}

// TODO: Criar a função print_transaction (para requisições)
// [cite: 119] Formato: YYYY-MM-DD HH:MM:SS client [IP] id req [ID] ...


int main(int argc, char **argv){
    int port;

    if (argc < 2){
        std::cout << "Porta nao informada. Uso: ./servidor <porta>" << std::endl;
        return 1;
    }
    port = atoi(argv[1]);

    int num_transactions = 0;
    int total_transfered = 0;
    int total_balance = 0;

    // Correção: Use {} para inicializar, () é declaração de função
    std::unordered_map<uint32_t, Client> clients{};
    std::list<Transaction> transaction_history{};

    // --- Configuração do Socket Servidor ---
    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[sizeof(packet)];

    // 1. Criar Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERRO: Nao foi possivel abrir o socket." << std::endl;
        return 1;
    }

    // 2. Configurar Endereço do Servidor
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Escutar em todas as interfaces
    serv_addr.sin_port = htons(port);

    // 3. Bind
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERRO: Nao foi possivel fazer o bind na porta " << port << std::endl;
        return 1;
    }

    std::cout << "Servidor escutando na porta " << port << std::endl;
    // Imprime o status inicial obrigatório [cite: 114]
    print_info(num_transactions, total_transfered, total_balance);


    // --- Loop Principal do Servidor ---
    while (true) {
        // 4. Receber Pacote
        int n = recvfrom(sockfd, buffer, sizeof(packet), 0,
                         (struct sockaddr *) &client_addr, &client_len);
        if (n < 0) {
            std::cerr << "ERRO: recvfrom" << std::endl;
            continue;
        }

        packet* p = (packet*) buffer;
        uint32_t client_ip = client_addr.sin_addr.s_addr; // IP em network byte order

        // 5. Processar Pacote
        if (p->type == DESC) {
            // [cite: 30] Lógica de Descoberta
            
            // TODO: Adicionar lock de Leitor/Escritor aqui (antes de acessar 'clients')
            
            bool new_client = false;
            if (clients.find(client_ip) == clients.end()) {
                // Cliente novo, registrar na tabela [cite: 30]
                Client client_obj(client_ip, INITIAL_BALANCE);
                clients[client_ip] = client_obj;
                total_balance += INITIAL_BALANCE;
                new_client = true;
                
                // Log interno do servidor (opcional, bom para debug)
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
                std::cout << "INFO: Cliente " << ip_str << " registrado. Saldo inicial: " << INITIAL_BALANCE << std::endl;
            }
            // TODO: Adicionar unlock aqui

            // 6. Responder com DESC_ACK (unicast) [cite: 30]
            packet ack_packet;
            ack_packet.type = DESC_ACK;
            ack_packet.seq_n = 0; // Não relevante para descoberta

            n = sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
                       (struct sockaddr *) &client_addr, client_len);
            if (n < 0) {
                std::cerr << "ERRO: sendto ACK de descoberta" << std::endl;
            }

            if(new_client){
                // TODO: A thread de interface (leitor) deve ser notificada
                // Por enquanto, apenas imprimimos o status atualizado
                print_info(num_transactions, total_transfered, total_balance);
            }

        } else if (p->type == REQ) {
            // TODO: Lógica de Requisição 
            // 1. Criar uma nova thread para processar 
            // 2. Dentro da thread:
            //    - Pegar lock de escrita
            //    - Validar saldo, processar, atualizar tabelas 
            //    - Liberar lock
            //    - Enviar REQ_ACK [cite: 38]
            //    - Chamar print_transaction(...)
            
        } else {
            // Pacote desconhecido ou antigo, ignorar
        }
    }

    close(sockfd);
    return 0;
}