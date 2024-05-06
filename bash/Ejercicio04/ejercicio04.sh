#!/bin/bash

# =========================== Encabezado =======================

# Nombre del script: Ejercicio4.sh
# Número de ejercicio: 4
# Trabajo Práctico: 1
# Entrega: Primera entrega

# ==============================================================

# ------------------------ Integrantes ------------------------
# 
#
# -------------------------------------------------------------

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
		echo "Error: sin permisos de lectura en directorio de salida"
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
				if [[ "$accion" == "MODIFY" ]]
				then
					path_comprimido="$2/$fecha.tar.gz"
					touch "$pathlog"
					tar -cf "$path_comprimido" --files-from /dev/null
					linea_coincidente=$(grep -n "$3" "$arch")
					if [ -n "$linea_coincidente" ]; then
						echo "El patrón fue encontrado en el archivo: $arch cuya coincidencia fue $linea_coincidente" >> "$pathlog"
						cp "$arch" "$2"
						tar -rf "$path_comprimido" "$2/$(basename "$arch")"
					else					
						echo "No hubo coindidencia alguna con el patrón." >> "$pathlog"
						tar -rf "$path_comprimido" "$2/$(basename "$pathlog")"
					fi
					gzip "$path_comprimido"
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

nombreScript=$(readlink -f $0)
dir_base=`dirname "$nombreScript"`
posi="$1"
if [[ "$1" == "-nohup-" ]]; 
then
	shift;	##borra el nohup y corre las demas variables una posición.
else # si no es igual a -nohup- significa que es la primer vuelta y apenas se comenzo a ejecutar el proceso por lo que aqui es donde tengo qeu crear las fifos donde se almacenaran la fecha inicial.
	if [[ (("$1" == "-d" || "$1" == "--directorio") && $# != 3 ) && ( ("$1" == "-d" || "$1" == "--directorio") && $# != 6) ]]
	then
		echo "Error: cantidad de parametros errónea"
		exit 1;
	fi
	if [[ ("$1" == "-h" || "$1" == "--help" || "$1" == "-?" ) && $# != 1 ]]
	then
		echo "Error: cantidad de parametros errónea"
		exit 1;
	fi
fi


case "$1" in
  '-d' | '--directorio')
	if [[ "$posi" != "-nohup-" ]]; 
	then
		#por primera ves debería pasar por aca.
		if [[ $# == 3 ]]
		then
			if [ ! -d "$2" ];
			then
				echo "Error: \"$2\" no es un directorio"
				mostrarAyuda
				exit 1;
			fi
			eliminarDemonioUnDirectorio "$2"
			#eliminarDemonio
		else
			validarParametros "$2" "$3" "$4" "$5" "$6"
			iniciarDemonio "-nohup-" $1 "$2" $3 "$4" $5 "$6"
		fi
	else
			#por aqui pasa la segunda
			#-d dirMonitoreo DirSalida patron
			iniciarDemonio "$1" "$2" "$4" "$5"
	fi
    ;;
  '-h' | '--help' | '-?')
	mostrarAyuda
	exit 1
    ;;
  *)
  echo "Obtener ayuda $0 [ -h | -help | -? ]"
esac