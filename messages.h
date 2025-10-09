#include <stdint.h>

struct request{
    uint32_t dest_addr; // Endereço IP do cliente de destino
    uint32_t amount;    // Valor da transferência
};

struct requestACK{
    uint32_t seq_n; // Número de sequência da mensagem do ACK
    uint32_t new_balance; // Novo saldo do cliente de origem
};

struct packet{
    uint16_t type;      // Tipo da requisição
    uint32_t seq_n;     // Número de sequência da requisição
    union{
        request req;
        requestACK ack;
    };
};