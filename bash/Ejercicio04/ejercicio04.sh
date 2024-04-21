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

	if [[ "$2" != "-s" && "$2" != "--salida"]];
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

	if [[ "$4" != "-p" && "$4" != "--patron"]];
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
			if [[ -f "$arch"]]
			then
				fecha=$(date +"%Y%m%d-%H%M%S")
				pathlog="$2/log_$fecha.txt"
				if[[ "$accion" == "MODIFY"]];
				then
					path_comprimido="$2/$fecha.tar.gz"
					touch "$pathlog"
					tar -czf "$path_comprimido" --files-from /dev/null
					if grep -q "patron_a_buscar" "$archivo";
					then
						echo "El patrón fue encontrado en el archivo: $archivo" >> "$pathlog"
						cp "$arch" "$2"
						tar -rf "$path_comprimido" "$2/$(basename "$archivo")"
					fi
				else
					echo "$archivo ha sido creado" >> "$pathlog"
				fi
			fi
   		done
   		sleep 20
  	done
}

existe() {
	#obtengo los demonios creados
	demonios=($(find "$dir_base" -name "*.pid"  -type f))
	
	declare -a vector
	posvec=0
	pos=0
	cadenas=""
	while [ $pos -lt ${#demonios[@]} ];do
		if [[ "${demonios[$pos]}" =~ .*.pid ]];
		then
				cadenas="$cadenas ${demonios[$pos]}"
				vector[$posvec]="$cadenas"
				let posvec=$posvec+1
				cadenas=""
		else
			if [[ $pos == 0 ]]
			then
				cadenas="${demonios[$pos]}"
			else
				cadenas="$cadenas ${demonios[$pos]}"
			fi
		fi
		let pos=$pos+1
	done

	pos=0
	while [ $pos -lt ${#vector[@]} ];do
		str="${vector[$pos]}"
		aux=${str:0:1}
		if [[ $aux == " " ]];
		then
			str=${str/" "/""}
			vector[$pos]="$str"
		fi
		let pos=$pos+1
	done

	inicio=0
	while [ $inicio -lt ${#vector[@]} ];do
   		#filtro
   		cosas=($(ps aux | grep `cat "${vector[$inicio]}"` | grep -v grep))
   		#comparo directorio monitorizado con directorio monitorizado.
		num=14
		directorio=""
		while [ "${cosas[$num]}" != "-a" ];do
			if [[ $num == 14 ]];
			then
				directorio="${cosas[$num]}"
			else
				directorio="$directorio ${cosas[$num]}"
			fi
			let num=$num+1
		done

		#¿readlinks de estos dos parametros de abajo?
		rutaabs1=`readlink -e "$1"`
		rutaabs2=`readlink -e "$directorio"`

		if [[ "$rutaabs1" == "$rutaabs2" ]];
   		then
   			return 1 #existe
   		fi
   		let inicio=$inicio+1
   	done
   	return 0
}

iniciarDemonio() { 
	if [[ "$1" == "-nohup-" ]];then
		#por aca pasa la primera ejecución, abriendo mi script en segundo plano...
  	 	existe "$3"
 		if [[ $? -eq 1 ]];then
    	 	 echo "el demonio ya existe"
    	  	exit 1;
		fi
		#lanzamos el proceso en segundo plano
		nohup bash $0 "$1" "$2" "$3" "$4" "$5" "$7" 2>/dev/null &
	else
		#por aca debería pasar la segunda ejecución pudiendo almacenar el PID del proceso para luego eliminarlo.
		#Tambien se inicia el loop
   	    pidFile="$dir_base/$$.pid";
		touch "$pidFile"
   	    echo $$ > "$pidFile"
		loop "$2" "$3" "$4"
	fi
}

#esta función solo se ejecutara cuando envie el parametro -k o --kill
eliminarDemonio() {
	#Obtenemos todos los demonios creados en anteriores scripts
	demonios=($(find "$dir_base" -name "*.pid" -type f))
	if [[ "${#demonios[*]}" == 0 ]];
	then
		echo "No hay demonios que eliminar..."
		exit 1
	fi

	declare -a vector
	posvec=0
	pos=0
	

	while [ $pos -lt ${#demonios[@]} ];do
		if [[ "${demonios[$pos]}" =~ .*.pid ]];
		then
				cadenas="$cadenas ${demonios[$pos]}"
				vector[$posvec]="$cadenas"
				let posvec=$posvec+1
				cadenas=""
		else
			if [[ $pos == 0 ]]
			then
				cadenas="${demonios[$pos]}"
			else
				cadenas="$cadenas ${demonios[$pos]}"
			fi
		fi
		let pos=$pos+1
	done

	pos=0
	while [ $pos -lt ${#vector[*]} ];do
		str="${vector[$pos]}"
		aux=${str:0:1}
		if [[ $aux == " " ]];
		then
			str=${str/" "/""}
			vector[$pos]="$str"
		fi
		let pos=$pos+1
	done
	
	
	inicio=0

	while [ $inicio -ne "${#vector[*]}" ];do
		cosas=($(ps aux | grep `cat "${vector[$inicio]}"` | grep -v grep))
		directorio=""
		num=14
		while [ "${cosas[$num]}" != "-a" ];do
			if [ $num == 14 ];
			then
				directorio="${cosas[$num]}"
			else
				directorio="$directorio ${cosas[$num]}"
			fi
			let num=$num+1
		done
		psino=($(ps aux | grep "inotifywait" | grep "$directorio"))
		kill `cat "${vector[$inicio]}"` 2>/dev/null
		kill "${psino[1]}" 2>/dev/null
		true > "${vector[$inicio]}"
		rm "${vector[$inicio]}"
		let inicio=$inicio+1
	done
}


eliminarDemonioUnDirectorio() {
    # Directorio a monitorear pasado como argumento
    dir_monitoreado="$1"
    
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
        
        # Verificar si la línea del comando contiene el directorio monitoreado
        if [[ "$ps_line" == *"inotifywait"* && "$ps_line" == *"$dir_monitoreado"* ]]; then
            echo "Eliminando demonio que monitorea: $dir_monitoreado"
            kill $pid 2>/dev/null
            true > "$pid_file"
            rm "$pid_file"
        fi
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
	if [[ (("$1" == "-d" || "$1" == "--directorio") && $# != 3 ) && ( ("$1" == "-d" || "$1" == "--directorio") && $# != 6) && ( ("$1" == "-h" || "$1" == "--help" || "$1" == "-?" ) && $# != 1) ]]
	then
		echo "Error: cantidad de parametros errónea"
		mostrarAyuda
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
