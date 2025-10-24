#include <stdint.h>

enum class messageType{
    DISCOVERY       = 0,
    DISCOVERY_ACK   = 1,
    REQUEST         = 2,
    REQUEST_ACK     = 3
};

struct request{
    uint32_t dest_addr; // Endereço IP do cliente de destino
    uint32_t amount;    // Valor da transferência
};

struct requestACK{
    uint32_t seq_n; // Número de sequência da mensagem do ACK
    uint32_t new_balance; // Novo saldo do cliente de origem
};

struct discoveryACK{
    uint32_t client_balance; // Saldo do cliente recém descoberto
};

typedef struct packet{
    uint16_t type;      // Tipo da requisição
    uint32_t seq_n;     // Número de sequência da requisição
    union{
        discoveryACK disc_ack;
        request req;
        requestACK req_ack;
    };
}Packet;