#include <iostream>
#include <thread>
#include <mutex>
#include <ctime>
#include <unordered_map>
#include <list>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include "messages.h"

// Saldo inicial de cada cliente descoberto pelo servidor
#define STARTING_BALANCE 10000

class Client{
    uint32_t ip_addr;
    int last_req;
    int balance;

    public:
    Client(uint32_t ip, int bal){
        this->ip_addr = ip;
        this->balance = bal;
        this->last_req = 0;
    }

    Client(){}

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

// Tabela Hash dos clientes
std::unordered_map<uint32_t, Client> clients;
// Histórico de transações
std::list<Transaction> transaction_history;
// TODO: Implementar os mutexes para leitura e escrita na tabela dos clientes e historico de transacao

// Função auxiliar para imprimir as informações na tela
void print_info(time_t time, int num_transactions, int total_transfered, int total_balance){
    std::string date_time(ctime(&time));

    std::cout << date_time << "num_transactions " << num_transactions << " total_transfered "
    << total_transfered << " total_balance " << total_balance;
}

void handle_discovery(int sock, sockaddr_in cli_addr){
    // Extraíndo o endereço IP do cliente
    uint32_t cli_ip = cli_addr.sin_addr.s_addr;
    // Adiciona novo cliente na tabela Hash de clientes
    Client new_cli(cli_ip, STARTING_BALANCE);
    clients[cli_ip] = new_cli;


    // Constroí o pacote de resposta
    packet discovery_answer;

    discovery_answer.type = static_cast<uint16_t>(messageType::DISCOVERY_ACK);
    discovery_answer.seq_n = 0;
    struct discoveryACK discACK = {static_cast<uint32_t>(new_cli.getBalance())};
    discovery_answer.disc_ack = discACK;

    // Enviando confirmação de descoberta
    sendto(sock, &discovery_answer, sizeof(discovery_answer), 0, (sockaddr*)&cli_addr, sizeof(cli_addr));
}

// Função para responder à uma requisição de um cliente
void handle_request(int sock, sockaddr_in cli_addr, packet req_message){
    // Endereco de origem e destino da transferencia
    uint32_t cli_ip = cli_addr.sin_addr.s_addr;
    uint32_t target_ip = req_message.req.dest_addr;

    // Checa se o cliente ja foi descoberto
    if (!clients.count(cli_ip) || !clients.count(target_ip))
        return;
    Client origin_cli = clients[cli_ip];
    // Checa se o numero de sequencia da requisição está correto
    if (origin_cli.getLastRequestID() != req_message.seq_n)
        return;
    
    // Checa se o cliente tem saldo suficiente
    int trans_val = req_message.req.amount;
    if (origin_cli.getBalance() < trans_val)
        return;
    
    // Atualiza o saldo dos 2 clientes
    origin_cli.updateBalance(-trans_val);
    clients[target_ip].updateBalance(trans_val);
    clients[cli_ip] = origin_cli;

    // Envia mensagem de confirmacao de transacao
    packet req_answer;
    req_answer.type = static_cast<uint16_t>(messageType::REQUEST_ACK);
    req_answer.seq_n = req_message.seq_n + 1;
    struct requestACK requestACK = {req_message.seq_n, static_cast<uint32_t>(origin_cli.getBalance())};
    req_answer.req_ack = requestACK;

    sendto(sock, &req_answer, sizeof(req_answer), 0, (sockaddr*)&cli_addr, sizeof(&cli_addr));
}

int main(int argc, char **argv){
    // Recebe a porta na qual o servidor irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Port not informed.\n";
        return 1;
    }
    int port = atoi(argv[1]);

    int num_transactions = 0;
    int total_transfered = 0;
    int total_balance = 0;

    time_t now = time(0);
    print_info(now, num_transactions, total_transfered, total_balance);

    // Abrindo a socket do servidor
    int sock;
    socklen_t clilen;
    struct sockaddr_in cli_addr, server_addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::cout << "ERROR on opening\n";
        return 1;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0){
        std::cout << "ERROR on bind\n";
        return 1;
    }

    clilen = sizeof(struct sockaddr_in);

    char buffer[256];
    int n;
    while (true){
        bzero(buffer, 256);
        n = recvfrom(sock, buffer, 256, 0, (struct sockaddr*)&cli_addr, &clilen);
        if (n < 0)
            continue;
        
        packet p = *(packet*)buffer;
        if (p.type == static_cast<uint16_t>(messageType::DISCOVERY)){
            std::thread discovery_thread(handle_discovery, sock, cli_addr);
            discovery_thread.detach();
        }
        else if (p.type == static_cast<uint16_t>(messageType::REQUEST)){
            std::thread request_thread(handle_request, sock, cli_addr, p);
            request_thread.detach();
        }
    }

    return 0;
}