all: cliente servidor

servidor:
	g++ ServidorConParametros.cpp -o Servidor -lpthread -lrt

cliente: 
	g++ ClienteConParametros.cpp -o Cliente -lpthread -lrt

clean:
	rm Servidor
	rm Cliente