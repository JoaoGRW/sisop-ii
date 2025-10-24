#include <iostream>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include "messages.h"

uint32_t curr_balance;
uint32_t curr_seq_n = 0;

void print_to_screen(uint32_t serv_address){
    time_t now = time(0);
    std::string date_time(ctime(&now));

    std::cout << date_time << " serv_addr " << serv_address; 

    // Espera recebimento de confirmação de transação e imprime os dados do cliente na tela
    
}

int main(int argc, char** argv){
    // Recebe a porta na qual o cliente irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Porta não informada.";
        return 1;
    }
    int port = atoi(argv[1]);
    uint32_t transfer_target_ip;
    int transfer_value;

    

    return 0;
}