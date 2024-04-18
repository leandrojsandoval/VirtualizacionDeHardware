#!/bin/bash

#########################################################
#			    Virtualizacion de hardware		        #
#	APL1 - Ejercicio 1		                            #
#	Nombre del script: Ejercicio1.sh	                #
#							                            #
#	Integrantes:		                                #
#         		                                        #
#       Ocampo, Nicole                      44451238	#
#       Sandoval Vasquez, Juan Leandro      41548235    #
#							                            #
#	Instancia de entrega:  Entrega		                #
#							                            #
#########################################################

function mostrar_ayuda() {
    echo "Uso: $0 -m1/--matriz1 <archivo> -m2/--matriz2 <archivo> -s/--separador <separador>"
    exit 1
}


multiplicar_matrices() {
    # Obtener el nombre de los archivos de las matrices
    matriz1_file="$1"
    matriz2_file="$2"
    separador="$3"
    declare -A resultado
    es_cuadrada=false
    es_fila=false
    es_columna=false

    #Comprobación de la compatibilidad para la multiplicación
    filas_matriz1=$(wc -l < "$matriz1_file")
    columnas_matriz1=$(head -n 1 "$matriz1_file" | tr -cd "$separador" | wc -c)
    filas_matriz2=$(wc -l < "$matriz2_file")
    columnas_matriz2=$(head -n 1 "$matriz2_file" | tr -cd "$separador" | wc -c)

    ((columnas_matriz1++))
    ((columnas_matriz2++))
    
    # Verificar si las dimensiones son compatibles para la multiplicación
    if (( columnas_matriz1 != filas_matriz2 )); then
        echo "No se pueden multiplicar las matrices: el número de columnas de la primera matriz no coincide con el número de filas de la segunda matriz."
        exit 1
    fi

    
    ###------------------ LOGICA DE MULTIPLICACION ENTRE LAS DOS MATRICES----------------------- ###

     # Inicializar la matriz resultado
    for ((i = 0; i <= columnas_matriz1; i++)); do
        for ((j = 0; j <= filas_matriz2; j++)); do
            resultado[$i,$j]=0
        done
    done

    # guardar matriz 1
    declare -A m1
    # Inicializar el contador de fila
    contador_fila=0
    # Leer el archivo línea por línea
    while IFS= read -r linea; do
        # Separar la línea en elementos usando la coma como delimitador
        IFS=$separador read -r -a elementos <<< "$linea"

        # Obtener el número de elementos en la línea
        num_elementos=${#elementos[@]}
        
        # Agregar los elementos a la matriz bidimensional
        for ((i=0; i<num_elementos; i++)); do
            m1[$contador_fila,$i]=${elementos[$i]}
        done
        
        # Incrementar el contador de fila
        ((contador_fila++))
    done < "$matriz1_file"

    echo "La matriz 1 es:"
    for ((i=0; i<contador_fila; i++)); do
        for ((j=0; j<num_elementos; j++)); do
            # Mostrar el elemento de la matriz con coma como separador
            echo -n "${m1[$i,$j]}"
            # Agregar coma si no es el último elemento de la fila
            if ((j < num_elementos - 1)); then
                echo -n ", "
            fi
        done
        echo ""  # Nueva línea para la siguiente fila
    done

    # guardar matriz 2

    declare -A m2
    # Inicializar el contador de fila
    contador_fila=0
    # Leer el archivo línea por línea
    while IFS= read -r linea; do
        # Separar la línea en elementos usando la coma como delimitador
        IFS=$separador read -r -a elementos <<< "$linea"
        
        # Obtener el número de elementos en la línea
        num_elementos=${#elementos[@]}
        #echo "Num Elementos:" $num_elementos
        
        # Agregar los elementos a la matriz bidimensional
        for ((i=0; i<num_elementos; i++)); do
            m2[$contador_fila,$i]=${elementos[$i]}
        done
        
        # Incrementar el contador de fila
        ((contador_fila++))
    done < "$matriz2_file"

    echo "La matriz 2 es:"
    for ((i=0; i<contador_fila; i++)); do
        for ((j=0; j<num_elementos; j++)); do
            # Mostrar el elemento de la matriz con coma como separador
            echo -n "${m2[$i,$j]}"
            # Agregar coma si no es el último elemento de la fila
            if ((j < num_elementos - 1)); then
                echo -n ", "
            fi
        done
        echo ""  # Nueva línea para la siguiente fila
    done

   
    # Multiplicación de matrices
    for ((i = 0; i < filas_matriz1; i++)); do
        for ((j = 0; j < columnas_matriz2; j++)); do
            suma=0
            for ((k = 0; k < columnas_matriz1; k++)); do
                suma=$((suma + ${m1[$i,$k]} * ${m2[$k,$j]}))
                # echo "i:" $i
                # echo "j:" $j
                # echo "k:" $k
                # echo "suma:" $suma
            done
            resultado[$i,$j]=$suma
            #echo "resultado[$i,$j]:" "${resultado[$i,$j]}"
        done
    done


    # Mostrar el resultado
    echo "La matriz resultante es:"
    for ((i=0; i<filas_matriz1; i++)); do
        for ((j=0; j<columnas_matriz2; j++)); do
            # Mostrar el elemento de la matriz con coma como separador
            echo -n "${resultado[$i,$j]}"
            # Agregar coma si no es el último elemento de la fila
            if [ $j -lt $((columnas_matriz2 - 1)) ]; then
                echo -n "$separador"
            fi
        done
        echo ""  # Nueva línea para la siguiente fila
    done
    

    ### ---------------------- LOGICA O FUNCIONES PARA SABER SI LA MATRIZ RESULTANTE ES FILA, COLUMNA, CUADRADA -----------------### 
    
     ##verifico si es cuadrada
    if [ "$filas_matriz1" -eq "$columnas_matriz2" ]; then
        es_cuadrada=true
    else
        es_cuadrada=false
    fi

    # Verificar si es fila o columna
    if [ "$filas_matriz1" -eq 1 ]; then
        es_fila=true
    fi

    if [ "$columnas_matriz2" -eq 1 ]; then
        es_columna=true
    fi

  
    echo "Orden de la matriz: $filas_matriz1 x $columnas_matriz2"
    echo "Es cuadrada: $es_cuadrada"
    echo "Es fila: $es_fila"
    echo "Es columna: $es_columna"

}


# Parsear argumentos
separador=","

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -m1|--matriz1)
            matriz1="$2"
            shift
            shift
            ;;
        -m2|--matriz2)
            matriz2="$2"
            shift
            shift
            ;;
        -s|--separador)
            separador="$2"
            shift
            shift
            ;;
        -h|--help)
            mostrar_ayuda
            ;;
        *)
            echo "Argumento desconocido: $1"
            exit 1
            ;;
    esac
done


# Verificar que se han proporcionado ambos archivos
if [ -z "$matriz1" ] || [ -z "$matriz2" ]; then
    echo "Se deben proporcionar las rutas de ambos archivos de matriz."
    exit 1
fi

# Verificar que los archivos existen
if [ ! -f "$matriz1" ] || [ ! -f "$matriz2" ]; then
    echo "Al menos uno de los archivos de matriz no existe."
    exit 1
fi

# verifico si el archivo no esta vacio
if [ ! -s "$matriz1" ] || [ ! -s "$matriz2" ]; then
    echo "Al menos uno de los archivos esta vacio."
    exit 1
fi
#verifico que el caracter separador no sea numero ni menos
if [[ "$separador" =~ [0-9] ]] || [[ "$separador" == "-" ]] || [[ "$separador" == "." ]]; then
    echo "El carácter separador '$separador' no es válido."
    exit 1
fi

multiplicar_matrices "$matriz1" "$matriz2" "$separador"



