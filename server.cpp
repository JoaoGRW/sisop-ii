#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
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
    uint32_t last_req;
    uint32_t balance;

    public:
    Client(uint32_t ip, uint32_t bal){
        this->ip_addr = ip;
        this->balance = bal;
        this->last_req = 0;
    }

    Client(){}

    // Metodos getters
    uint32_t getIPAddress(){ return this->ip_addr; }

    uint32_t getLastRequestID(){ return this->last_req; }

    uint32_t getBalance(){ return this->balance; }

    void updateBalance(int amount_transfered){
        this->balance += amount_transfered;
    }

    void updateLastRequestID(){
        this->last_req++;
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
std::mutex clients_mtx;
// Histórico de transações
std::list<Transaction> transaction_history;
std::mutex transactions_mtx;
int next_transaction_id = 0;

// Variáveis de controle de novas entradas nas estruturas
bool newTransaction = false;
int num_transactions = 0;
int total_transfered = 0;
int total_balance = 0;

// Mutex e variável de condição para o subsistema de interface
std::condition_variable cv;
std::mutex cv_mtx;

// Função auxiliar para criar um cliente e adicioná-lo na tabela
Client create_new_client(uint32_t clientIP, uint32_t startingBalance){
    Client new_client(clientIP, startingBalance);
    clients[clientIP] = new_client;
    total_balance += startingBalance;

    return new_client;
}

// Função que será executada por uma thread a cada nova mensagem de descoberta
void handle_discovery(int sock, sockaddr_in server_addr, sockaddr_in cli_addr){
    // Extraíndo o endereço IP do cliente e checando se ele já não está na tabela
    uint32_t cli_ip = cli_addr.sin_addr.s_addr;
    Client client;

    // Ínicio da seção crítica
    clients_mtx.lock(); 
    // Checa se o cliente já não está na tabela de clientes
    if (!clients.count(cli_ip)){
        // Se não está cria novo e adiciona na tabela
        client = create_new_client(cli_ip, STARTING_BALANCE);
    }else{
        client = clients[cli_ip];
    }
    clients_mtx.unlock();
    // Fim da seção crítica

    // Constroí o pacote de resposta
    packet discovery_answer;
    discovery_answer.type = static_cast<uint16_t>(messageType::DISCOVERY_ACK);
    discovery_answer.seq_n = 0;
    struct discoveryACK discACK = { server_addr.sin_addr.s_addr, client.getBalance()};
    discovery_answer.disc_ack = discACK;

    // Enviando confirmação de descoberta
    sendto(sock, &discovery_answer, sizeof(discovery_answer), 0, (sockaddr*)&cli_addr, sizeof(cli_addr));
}

// Função auxiliar para enviar um ACK de uma requisição
void send_request_ACK(Client& client, int sock, sockaddr_in client_addr){
    packet reqACK;
    reqACK.type = static_cast<uint16_t>(messageType::REQUEST_ACK);
    reqACK.seq_n = client.getLastRequestID();
    struct requestACK ack = { reqACK.seq_n, client.getBalance() };

    // Envia o ACK da requisição
    sendto(sock, &reqACK, sizeof(reqACK), 0, (sockaddr*)&client_addr, sizeof(&client_addr));
}

// Função para responder à uma requisição de um cliente
void handle_request(int sock, sockaddr_in cli_addr, packet req_message){
    // Endereco de origem e destino da transferencia
    uint32_t cli_ip = cli_addr.sin_addr.s_addr;
    uint32_t dest_ip = req_message.req.dest_addr;

    // Ínicio da seção crítica
    clients_mtx.lock();
    // Checa se o cliente ja foi descoberto
    if (!clients.count(cli_ip))
        return;
    Client origin_cli = clients[cli_ip];

    // Checa se o cliente destino já foi criado 
    Client dest_cli;
    if (!clients.count(dest_ip))
        dest_cli = create_new_client(dest_ip, STARTING_BALANCE);
    else
        dest_cli = clients[dest_ip]; 
    clients_mtx.unlock();
    // Fim da seção crítica

    // Checa se o numero de sequencia da requisição está correto
    // Caso não esteja o correto o número de sequência o servidor reenvia o último ACK
    if (origin_cli.getLastRequestID() != req_message.seq_n){
        send_request_ACK(origin_cli, sock, cli_addr);
    }
    
    // Checa se o cliente tem saldo suficiente
    int transaction_val = req_message.req.amount;
    if (origin_cli.getBalance() < transaction_val)
        return;
    
    // Atualiza o saldo dos 2 clientes e envia a mensagem de ACK
    origin_cli.updateBalance(-transaction_val);
    dest_cli.updateBalance(transaction_val);
    origin_cli.updateLastRequestID();

    // Ínicio da seção crítica
    clients_mtx.lock();

    clients[cli_ip] = origin_cli;
    clients[dest_ip] = dest_cli;

    clients_mtx.unlock();
    // Fim da seção crítica

    send_request_ACK(origin_cli, sock, cli_addr);

    // Atualiza o histórico de transferências
    // Ínicio da seção crítica
    transactions_mtx.lock();

    Transaction transaction(cli_ip, next_transaction_id, dest_ip, transaction_val);
    next_transaction_id++;
    transaction_history.push_back(transaction);

    transactions_mtx.unlock();
    // Fim da seção crítica

    // Atualiza os dados de transação e notifica a thread de interface da mudança
    std::lock_guard<std::mutex> lock(cv_mtx);
    newTransaction = true;

    num_transactions++;
    total_transfered += transaction_val;

    cv.notify_one();
}

// Função auxiliar que imprime YYYY-MM-DD hh:mm:ss
void print_current_date_time(){
    time_t now = time(0);
    struct tm *dt = localtime(&now);

    std::cout << dt->tm_year + 1900 << '-' << dt->tm_mon << '-' << dt->tm_mday << ' '
    << dt->tm_hour << ':' << dt->tm_min << ':' << dt->tm_sec;
} 

// Função auxiliar para imprimir dados de transação do servidor
void print_transactions_data(){
    std::cout << " num_transactions " << num_transactions << " total_transfered "
    << total_transfered << " total_balance " << total_balance << '\n';
}

// Função que será executada pela thread do sbuserviço de interface
void handle_interface(){
    
    print_current_date_time();
    print_transactions_data();

    // Imprime informações dos clientes e transações a cada atualização
    while(true){
        std::unique_lock<std::mutex> lock(cv_mtx);
        cv.wait(lock, [] { return newTransaction; });

        // Reseta a flag de nova transação
        newTransaction = false;

        // Le os dados da ultima transacao realizada
        transactions_mtx.lock();
        print_current_date_time();
        Transaction last_t = transaction_history.back();
        std::cout << " client " << last_t.getOriginIP() << " id_req " << last_t.getReqID()
        << " dest " << last_t.getDestIP() << " value " << last_t.getAmount() << '\n';
        transactions_mtx.unlock();

        print_transactions_data(); 

    }
}

// Função auxiliar para abertura e bind da socket do servidor
bool open_server_socket(int& sock, int port, struct sockaddr_in& server_addr){
    // Abrindo socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        std::cerr << "ERROR on opening\n";
        return false;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    // Bind socket
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0){
        std::cerr << "ERROR on bind\n";
        return false;
    }

    return true;
}

int main(int argc, char **argv){
    // Recebe a porta na qual o servidor irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Port not informed.\n";
        return 1;
    }
    int port = atoi(argv[1]);

    // Abrindo a socket do servidor
    int sock;
    struct sockaddr_in cli_addr, server_addr;

    if(!open_server_socket(sock, port, server_addr))
        return 1;

    // Thread do subserviço de interface
    std::thread interface_thread(handle_interface);

    socklen_t clilen;
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
            std::thread discovery_thread(handle_discovery, sock, server_addr, cli_addr);
            discovery_thread.detach();
        }
        else if (p.type == static_cast<uint16_t>(messageType::REQUEST)){
            std::thread request_thread(handle_request, sock, cli_addr, p);
            request_thread.detach();
        }
    }

    return 0;
}