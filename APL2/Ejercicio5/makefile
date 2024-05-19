all: cliente servidor disparador

servidor:
	g++ Servidor.cpp -o Servidor -lpthread -lrt

cliente: 
	g++ Cliente.cpp -o Cliente -lpthread -lrt
disparador:
	g++ Disparador.cpp -o Disparador -lpthread -lrt

clean:
	rm Servidor
	rm Cliente
	rm Disparador