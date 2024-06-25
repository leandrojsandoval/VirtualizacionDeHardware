all: cliente servidor disparador

servidor:
	g++ ServidorPrueba.cpp -o ServidorPrueba -lpthread -lrt

cliente: 
	g++ ClienteP.cpp -o ClienteP -lpthread -lrt
disparador:
	g++ Disparador.cpp -o Disparador -lpthread -lrt

clean:
	rm ServidorPrueba
	rm ClienteP
	rm Disparador