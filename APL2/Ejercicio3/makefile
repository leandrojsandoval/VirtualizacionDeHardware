# Regla por defecto para compilar los ejecutables
all: daemon sensor

# Regla para compilar el demonio
daemon: daemon.cpp
	g++ daemon.cpp -o daemon.exe

# Regla para compilar el sensor
sensor: sensor.cpp
	g++ sensor.cpp -o sensor.exe

# Regla para limpiar los ejecutables generados
clean:
	rm -f daemon.exe sensor.exe log.txt
