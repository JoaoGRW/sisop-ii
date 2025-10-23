#pragma once // Boa prática para evitar inclusões múltiplas
#include <stdint.h>

//  Tipos de pacote
enum PacketType {
    DESC = 1,     // Descoberta (Cliente -> Servidor)
    DESC_ACK = 2, // Resposta da Descoberta (Servidor -> Cliente)
    REQ = 3,      // Requisição de Transferência (Cliente -> Servidor)
    REQ_ACK = 4   // Resposta da Requisição (Servidor -> Cliente)
};

struct request{
    uint32_t dest_addr; // Endereço IP do cliente de destino
    uint32_t amount;    // Valor da transferência
};

struct requestACK{
    uint32_t seq_n; // Número de sequência da mensagem do ACK
    uint32_t new_balance; // Novo saldo do cliente de origem
};

struct packet{
    uint16_t type;      // Tipo da requisição (usando PacketType)
    uint32_t seq_n;     // Número de sequência da requisição
    union{
        request req;
        requestACK ack;
    };
};