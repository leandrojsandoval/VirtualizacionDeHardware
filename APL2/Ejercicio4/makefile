all: cliente servidor

servidor:
	g++ servidor.cpp -o servidor -lpthread -lrt

cliente: 
	g++ cliente.cpp -o cliente -lpthread -lrt


clean:
	rm servidor
	rm cliente
