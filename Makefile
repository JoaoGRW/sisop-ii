# Compilador C++
CPP = g++

# Flags de compilação g++
CPPFLAGS = -std=c++11 -pthread

# Nome dos executáveis gerados
SERVER_TARGET = server 
CLIENT_TARGET = client 

# Arquivos fonte
SERVER_SRC = server.cpp 
CLIENT_SRC = client.cpp

# Arquivos de cabeçalho necessários (headers)
HEADERS = messages.h 

# Compilação do servidor
$(SERVER_TARGET): $(SERVER_SRC) $(HEADERS)
	$(CPP) $(CPPFLAGS) $(SERVER_SRC) -o $(SERVER_TARGET)

# Compilação do cliente
$(CLIENT_TARGET): $(CLIENT_SRC) $(HEADERS)
	$(CPP) $(CPPFLAGS) $(CLIENT_SRC) -o $(CLIENT_TARGET)

# Regra make clean
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET) *.o