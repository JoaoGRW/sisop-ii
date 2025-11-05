# Compilador C++
CPP = g++

# Flags de compilação g++
CPPFLAGS = -std=c++11 -pthread

# Nome dos executáveis gerados
SERVER_EXE = server 
CLIENT_EXE = client 

# Arquivos fonte
SERVER_SRC = server.cpp 
CLIENT_SRC = client.cpp

# Arquivos de cabeçalho necessários (headers)
HEADERS = messages.h 

# Execução padrão compila os arquivos fonte do cliente e servidor
all: $(SERVER_EXE) $(CLIENT_EXE)

# Compilação do servidor
$(SERVER_EXE): $(SERVER_SRC) $(HEADERS)
	$(CPP) $(CPPFLAGS) $(SERVER_SRC) -o $(SERVER_EXE)

# Compilação do cliente
$(CLIENT_EXE): $(CLIENT_SRC) $(HEADERS)
	$(CPP) $(CPPFLAGS) $(CLIENT_SRC) -o $(CLIENT_EXE)

# Regra make clean
.PHONY: clean
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE) *.o