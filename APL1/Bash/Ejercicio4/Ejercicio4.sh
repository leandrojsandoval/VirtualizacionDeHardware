#!/bin/bash

#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL1 - Ejercicio 4                                  #
#   Nombre del script: Ejercicio4.sh                    #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Vivas, Pablo Ezequiel               38703964    #
#       Villegas, Lucas Ezequiel            37792844    #
#       Tigani, Martin Sebastian            32788835    #
#                                                       #
#   Instancia de entrega: Segunda Entrega               #
#                                                       #
#########################################################

validarParametros() { #directorio -s directoriosalida -p patron
	if [ ! -d "$1" ];
	then
		echo "Error: \"$1\" no es un directorio"
		mostrarAyuda
		exit 1;
	fi

	if [ ! -r "$1" ];
	then
		echo "Error: sin permisos de lectura en directorio a monitorear"
		mostrarAyuda
		exit 1;
	fi

	if [[ "$2" != "-s" && "$2" != "--salida" ]]
	then
		echo "Error, el tercer parametro debería de ser "-s" o "--salida""
		mostrarAyuda
		exit 1
	fi

	if [ ! -d "$3" ];
	then
		echo "Error: \"$3\" no es un directorio"
		mostrarAyuda
		exit 1;
	fi

	if [ ! -w "$3" ];
	then
		echo "Error: sin permisos de escritura en directorio de salida"
		mostrarAyuda
		exit 1;
	fi

	if [[ "$4" != "-p" && "$4" != "--patron" ]]
	then
		echo "Error, el quinto parametro debería de ser "-p" o "--patron""
		mostrarAyuda
		exit 1
	fi

	if [ ! -n "$5" ];
	then
		echo "Error, no hay patron que de busqueda"
		mostrarAyuda
		exit 1
	fi
	return 0
}

loop() {

   	while [[ true ]];do
   		inotifywait -r -m -e create -e modify --format "%e "%w%f"" "$1" | while read accion arch;do
			if [ -f "$arch" ];
			then
				fecha=$(date +"%Y%m%d-%H%M%S")
				pathlog="$2/log_$fecha.txt"
				touch "$pathlog"
				if [[ "$accion" == "MODIFY" ]]
				then
					path_comprimido="$2/$fecha.tar.gz"
					linea_coincidente=$(grep -n "$3" "$arch")
					if [ -n "$linea_coincidente" ]; then
						echo "El patrón fue encontrado en el archivo: $arch la coincidencia se ha guardado en $path_comprimido" >> "$pathlog"
						pathAuxPatron="$2/coincidencia$fecha.txt"
						echo "$linea_coincidente" >> "$pathAuxPatron"
						if [ ! -f "$path_comprimido" ]; then
    						tar -cvzf "$path_comprimido" "$pathAuxPatron" >/dev/null
						else
   							tar -rvzf "$path_comprimido" "$pathAuxPatron" >/dev/null
						fi
						rm "$pathAuxPatron"
					else					
						echo "No hubo coindidencia alguna con el patrón $3." >> "$pathlog"
					fi
				else
					echo "$arch ha sido creado" >> "$pathlog"
				fi
			fi
   		done
   		sleep 20
  	done
}

existeDemonio() {
	nombrescript=$(readlink -f $0)
	dir_base=`dirname "$nombrescript"`
    pid_files=($(find "$dir_base" -name "*.pid" -type f))
    local pid directorio procesos rutaabs1 rutaabs2
    rutaabs1=$(readlink -e "$1")
    if [[ -z "$rutaabs1" ]]; then
        echo "El directorio proporcionado no existe."
        return 2
    fi
    for pid_file in "${pid_files[@]}"; do
        pid=$(cat "$pid_file")
        procesos=$(ps -p "$pid" -o args=)
        if [[ "$procesos" =~ -d\ ([^\ ]*) ]]; then
            directorio="${BASH_REMATCH[1]}"
            rutaabs2=$(readlink -e "$directorio")
            if [[ "$rutaabs1" == "$rutaabs2" ]]; then
                echo "El directorio '$1' está siendo monitoreado por el proceso con PID $pid."
                return 1
            fi
        fi
    done
    echo "El directorio '$1' no está siendo monitoreado se procede con la ejecución."
    return 0
}


iniciarDemonio() { 
	if [[ "$1" == "-nohup-" ]];then
		#por aca pasa la primera ejecución, abriendo mi script en segundo plano...
  	 	existeDemonio "$3"
 		if [[ $? -eq 1 ]];then
    	 	echo "el demonio ya existe"
    	  	exit 1;
		fi
		#lanzamos el proceso en segundo plano
		nohup bash $0 "$1" "$2" "$3" "$4" "$5" "$6" "$7" 2>/dev/null &
	else
		nombrescript=$(readlink -f "$0")
		dir_base=$(dirname "$nombrescript")

		#por aca debería pasar la segunda ejecución pudiendo almacenar el PID del proceso para luego eliminarlo.
		#Tambien se inicia el loop
   	    pidFile="$dir_base/$$.pid";
		touch "$pidFile"
   	    echo $$ > "$pidFile"
		loop "$2" "$3" "$4"
	fi
}

eliminarDemonioUnDirectorio() {
    # Directorio a monitorear pasado como argumento
    dir_monitoreado=$(realpath "$1" | tr -d '[:space:]')
	echo "$dir_monitoreado"
	echo "$dir_base"
    # Obtenemos todos los demonios creados en anteriores scripts
    demonios=($(find "$dir_base" -name "*.pid" -type f))
    if [[ "${#demonios[*]}" == 0 ]]; then
        echo "No hay demonios que eliminar..."
        exit 1
    fi
    # Buscar y eliminar solo el demonio asociado con el directorio especificado
    for pid_file in "${demonios[@]}"; do
        # Extraer el proceso que corresponde al pid en el archivo pid
        pid=$(cat "$pid_file")
        # Usamos ps para obtener los detalles del comando que lanzó este pid
        ps_line=$(ps -p $pid -o cmd=)
		directorio_del_proceso=$(echo "$ps_line" | awk -v RS=" -" -F' ' '$1=="d"{for (i=2; i<=NF; i++) if ($i != "-s") printf "%s ", $i; else exit}')
		directorio_del_procesoCompleto=$(realpath "$directorio_del_proceso" | tr -d '[:space:]')
		# comparamos rutas
		if [[ "$directorio_del_procesoCompleto" == "$dir_monitoreado" ]]; then
            echo "Eliminando demonio que monitorea: $dir_monitoreado"
            kill $pid 2>/dev/null
            true > "$pid_file"
            rm "$pid_file"
        fi
		#Obtenemos la informacion correspondiente del inotifywait que se encuentra monitoreando el directorio.
		procesosInotify=$(ps -eo pid,cmd | grep "inotifywait")
		#loopeamos hasta encontrar el pid al que corresponda el directorio
		for pro_ino in "${procesosInotify[@]}"; do
			#obtenemos el directorio del inotify
			dir_inotify=$(echo "$pro_ino" | awk '/inotifywait/ && !/grep/ {for (i=1; i<=NF; i++) if ($i == "--format") {j=i+3; while (j<=NF) {printf "%s ", $j; j++}; print ""; exit}}')
			#obtenemos la ruta absoluta del comando
			dir_inotifyCompleto=$(realpath "$dir_inotify" | tr -d '[:space:]')
			if [[ "$dir_inotifyCompleto" == "$dir_monitoreado" ]]; then
				#Si ambas rutas son identicas buscamos el pid y luego lo eliminamos
				pid_inotify=$(echo "$pro_ino" | grep "inotifywait" | grep -v "grep" | awk '{print $1}')
				kill $pid_inotify 2>/dev/null
				exit 1
			fi
		done
    done
}


mostrarAyuda() {
	echo "Modo de uso: bash $0 [-d | --directorio] [Directorio de monitoreo] [ -s | --salida ] [Directorio de backup de archivos] [ -p | --patron ] [patron de busqueda]"
	echo ""
	echo "Monitorear un directorio para ver si hubo cambios en este"
	echo "-d | --directorio	indica el directorio a monitorear"
	echo "El directorio a monitorear indicado en -d puede estar vacío."
	echo "-s | --salida 	Directorio de salida."
	echo "-p | --patron 	patron a utilizar."
	echo "En caso de querer terminar el proceso escribimos bash $0 [-d | --directorio] [Directorio de monitoreo a detener] [-k | --kill]"
}

# Procesamiento de argumentos con getopt
if [[ "$1" == "-nohup-" ]]; then
    shift # Elimina el -nohup- de los argumentos
	iniciarDemonio "$1" "$2" "$4" "$6"
else
	#por aqui debería de pasar la primera vez. luego la segunda con el nohup
    TEMP=$(getopt -o d:s:p:hk --long directorio:,salida:,patron:,help,kill -n "$0" -- "$@")
    if [ $? != 0 ]; then
        echo "Error: fallo en la interpretacion de los argumentos" >&2
        exit 1
    fi
    eval set -- "$TEMP"
fi

while true; do
    case "$1" in
        -d|--directorio)
            DIRECTORIO="$2"
            shift 2
            ;;
        -s|--salida)
            SALIDA="$2"
            shift 2
            ;;
        -p|--patron)
            PATRON="$2"
            shift 2
            ;;
        -h|--help)
            mostrarAyuda
            exit 0
            ;;
        -k|--kill)
            KILL="true"
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Error: Parametro no reconocido $1"
            mostrarAyuda
            exit 1
            ;;
    esac
done


# Verificar si hay parámetros adicionales después de procesar getopt
if [[ $# -ne 0 ]]; then
    echo "Error: Parámetros adicionales no reconocidos: $@"
    mostrarAyuda
    exit 1
fi

#por aca solo pasa la primer ejecucion

nombreScript=$(readlink -f $0)
dir_base=`dirname "$nombreScript"`

if [[ "$KILL" == "true" ]]; then
	echo "$DIRECTORIO"
    eliminarDemonioUnDirectorio "$DIRECTORIO"
    exit 0
fi

if [[ -z "$DIRECTORIO" || -z "$SALIDA" || -z "$PATRON" ]]; then
    echo "Error: falta algún argumento obligatorio."
    mostrarAyuda
    exit 1
fi

validarParametros "$DIRECTORIO" "-s" "$SALIDA" "-p" "$PATRON"
iniciarDemonio "-nohup-" "-d" "$DIRECTORIO" "-s" "$SALIDA" "-p" "$PATRON"
