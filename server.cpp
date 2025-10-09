#include <iostream>
#include <thread>
#include <ctime>
#include <unordered_map>
#include <list>
#include "messages.h"

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

// Função auxiliar para imprimir as informações na tela
void print_info(time_t time, int num_transactions, int total_transfered, int total_balance){
    std::string date_time(ctime(&time));

    std::cout << date_time << "num_transactions " << num_transactions << " total_transfered "
    << total_transfered << " total_balance " << total_balance;
}

int main(int argc, char **argv){
    int port;

    // Recebe a porta na qual o servidor irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Porta nao informada.";
    }
    port = atoi(argv[1]);

    int num_transactions = 0;
    int total_transfered = 0;
    int total_balance = 0;

    // Inicializando o Hash Map de clientes e a lista do historico
    std::unordered_map<uint32_t, Client> clients();
    std::list<Transaction> transaction_history();

    time_t now = time(0);
    print_info(now, num_transactions, total_transfered, total_balance);

    return 0;
}