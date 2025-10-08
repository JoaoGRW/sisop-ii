#include <iostream>
#include <thread>
#include <unordered_map>
#include <list>

class Client{
    uint32_t ip_addr;
    int last_req;
    int balance;

    public:
    Client(uint32_t ip, int bal){
        this->ip_addr = ip;
        this->balance = bal;
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
    
};

int main(){
    int num_transactions = 0;
    int total_transfered = 0;
    int total_balance = 0;
    // Inicializados o Hash Map de clientes e a lista do historico
    std::unordered_map<uint32_t, Client> clients();
    std::list<Transaction> transaction_history();



    return 0;
}