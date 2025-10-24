#include <iostream>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include "messages.h"

int main(int argc, char** argv){
    // Recebe a porta na qual o cliente irá rodar pela linha de comando
    if (argc < 2){
        std::cout << "Porta não informada.";
        return 1;
    }
    int port = atoi(argv[1]);
    uint32_t transfer_target_ip;
    int transfer_value;

    std::cout << "Informe os dados da próxima transação:\n";
    std::cin >> transfer_target_ip >> transfer_value;
    std::cout << transfer_target_ip << " " << transfer_value;

    return 0;
}