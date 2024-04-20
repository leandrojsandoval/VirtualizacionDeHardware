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
	IFS=" "
	cadena="$2"
	cadena=${cadena//,/" "}
	acciones=($(echo "$cadena"))

	declare -A acc
	acc["listar"]=0
	acc["peso"]=0
	acc["compilar"]=0
	acc["publicar"]=0

	inicio=0
	#Usamos el array asociativo para insertar 1 en aquellas acciones que tenemos en la lista de acciones.
	while [ "$inicio" -ne "${#acciones[*]}" ];do
		acc["${acciones[$inicio]}"]=1
		let inicio=$inicio+1
	done

   	while [[ true ]];do
   		inotifywait -r -m -e create -e modify -e delete --format "%e "%w%f"" "$1" | while read accion arch;do
   			if [[ -f "$arch" || "$accion" == "DELETE" ]];
   			then
   				if [[ ${acc["listar"]} == 1 ]];
   				then
   					echo "$arch"
   				fi

   				if [[ ${acc["peso"]} == 1 && "$accion" != "DELETE" ]];
   				then
   					echo "$arch pesa `cat "$arch" | wc -c` bytes"
   				fi

   				if [[ ${acc["compilar"]} == 1 ]];
   				then
 					find "$1" -type f -exec cat '{}' \; | cat >> "$dir_base/bin/$(cat "$pidFile").txt"
				fi

				if [[ ${acc["publicar"]} == 1 ]];
				then
					cp "$dir_base/bin/$(cat "$pidFile").txt" "$3/$(cat "$pidFile").txt"
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
		if [[ $# == 7 ]];then # -nohup- -c dirMoni -a acciones dirDes
	   		nohup bash $0 "$1" "$2" "$3" "$4" "$5" "$7" 2>/dev/null &
	   	else # -nohup- -c dirMoni -a acciones
	   		nohup bash $0 "$1" "$2" "$3" "$4" "$5" 2>/dev/null &
	   	fi
	else
		#por aca debería pasar la segunda ejecución pudiendo almacenar el PID del proceso para luego eliminarlo.
		#Tambien se inicia el loop
   	    pidFile="$dir_base/$$.pid";
		touch "$pidFile"
   	    echo $$ > "$pidFile"
   	    if [[ $# == 4 ]];
   	    then #dirMoni acciones dirDes
   	    	loop "$2" "$3" "$4"
   		else #dirMoni acciones
   			loop "$2" "$3"
   		fi
	fi
}

#esta función solo se ejecutara cuando envie el parametro -d
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
	if [[ "$1" != "-k" && "$1" != "--kill" && "$1" != "-h" && "$1" != "--help" && "$1" != "-?" ]];
	then
		if [ ! -n "$2" ];
		then
			echo "ERROR: falta pasar directorio a monitorear"
			exit 1;
		fi
	else
		if [[ "$1" == "-k" && $# != 3 ]]
		then
			echo "Error, -k debe enviarse junto con el directorio a monitorear"
			echo "Ejemplo: $0 -d directorio -k"
			exit 1
		fi
		if [[ "$1" == "--kill" && $# != 3 ]]
		then
			echo "Error, -kill debe enviarse junto con el directorio a monitorear"
			echo "Ejemplo: $0 -d directorio -kill"
			exit 1
		fi
	fi
fi


case "$1" in
  '-d' | '--directorio')
	if [[ "$posi" != "-nohup-" ]]; 
	then
		#por primera ves debería pasar por aca.
		validarParametros "$2" "$3" "$4" "$5" "$6"
		if [[ $# == 6 ]];
		then
			let retorno=$?
			if [[ $retorno == 2 ]];
			then
				#si el directorio no existe mandare el directorio de la script.
				iniciarDemonio "-nohup-" $1 "$2" $3 "$4" $5 "$dir_base"
			else
				iniciarDemonio "-nohup-" $1 "$2" $3 "$4" $5 "$6"
			fi
		else
			iniciarDemonio "-nohup-" $1 "$2" "$3" "$4"
		fi
	else
			#por aqui pasa la segunda
			if [[ $# == 5 ]]; #en caso de que este -s más el directorio destino
			then #-c dirMoni acciones dirDes
    			iniciarDemonio "$1" "$2" "$4" "$5"
    		else	# en caso de que no este -s más el directorio destino -c dirMoni acciones
    			iniciarDemonio "$1" "$2" "$4"
    		fi
	fi
    ;;
  '-h' | '--help' | '-?')
	mostrarAyuda
	exit 1
    ;;
   '-k' | '--kill')
	eliminarDemonio
	;;
  *)
  echo "Obtener ayuda $0 [ -h | -help | -? ]"
esac
