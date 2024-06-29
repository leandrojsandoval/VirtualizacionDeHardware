#!/bin/bash
#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL1 - Ejercicio 5                                  #
#   Nombre del script: Ejercicio5.sh                    #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Vivas, Pablo Ezequiel               38703964    #
#       Villegas, Lucas Ezequiel            37792844    #
#       Tigani, Martin Sebastian            32788835    #
#                                                       #
#   Instancia de entrega: Primera Entrega               #
#                                                       #
#########################################################
# Función para imprimir la información de un personaje
getInfo() {
    local response=$1

    local length=$(echo "$response" | jq '. | length')



    if [[ $length -eq 0 ]]; then
        echo "Error: No se encontraron resultados para el nombre/nombres proporcionados."
        exit 1
    fi

    if [[ $length -eq 1 ]]; then
        jq -r '.[] | "Character info:\nId: \(.id)\nName: \(.name)\nStatus: \(.status)\nSpecies: \(.species)\nGender: \(.gender)\nOrigin: \(.origin.name)\nLocation:\(.location.name)\n"' <<< "$response"
    else

        jq -r '.[] | "Character info:\nId: \(.id)\nName: \(.name)\nStatus: \(.status)\nSpecies: \(.species)\nGender: \(.gender)\nOrigin: \(.origin.name)\nLocation:\(.location.name)\n"' <<< "$response"
    fi
}


mostrarAyuda() {
	echo "Modo de uso: bash $0 [-i | --id] ["1,2,3"] [ -n | --nombre ] ["rick, morty"]"
	echo ""
	echo "Uso de api y caches json"
	echo "-i | --id	indica los ids a consultar"
	echo "-n | --nombre indica los nombres a consultar."

}

# Función para verificar si un personaje con un ID específico está en caché
PersonajeCacheId() {
   
    local id=$1
    local filename="personaje_$id.json"
    if [[ -f "$filename" ]]; then
        local response=$(cat "$filename")
        getInfo "$response"
        exit 0  # Salir si el personaje está en la caché
    fi
}

# Función para verificar si un personaje con un nombre específico está en caché
PersonajeCacheName() {
    
    local name=$1
    local filename="personaje_$name.json"
    if [[ -f "$filename" ]]; then
        local response=$(cat "$filename")
        getInfo "$response"
        exit 0  # Salir si el personaje está en la caché
    fi
}

# Función para realizar la búsqueda HTTP de personajes por ID
PersonajeId() {

    local ids=$1
    IFS=',' read -r -a array <<< "$ids"

    # Contar la cantidad de elementos en el array
    count=${#array[@]}
  
    PersonajeCacheId $ids
    local url="https://rickandmortyapi.com/api/character/$ids"
    local response=$(wget -qO- "$url")

    if [[ $(echo "$response" | jq '. | length') -eq 0 ]]; then
        echo "Error: No se encontraron personajes con los IDs proporcionados."
        exit 1
    fi

     if [[ $count -eq 1 ]]; then
        declare -a response2
        response2+=("$response")
        echo "[$response2]" > "personaje_$ids.json"
        getInfo "[$response2]"
     else
         echo "$response" > "personaje_$ids.json"
         getInfo "$response"
     fi

   
    
}


# Función para realizar la búsqueda HTTP de personajes por nombre
PersonajeName() {
    local names=$1
    PersonajeCacheName $names
    IFS=',' read -r -a name_array <<< "$names"
    for name in "${name_array[@]}"; do
       # character_in_cache_by_name $name

        local url="https://rickandmortyapi.com/api/character/?name=$name"
        local response=$(wget -qO- "$url" | jq '.results')
        local length=$(echo "$response" | jq '. | length')

        # Verificar si la respuesta es una matriz vacía
        if [[ $length -eq 0 ]]; then
            echo "Error: No se encontraron resultados para el nombre '$name'."
        else
            echo "$response"> "personaje_$name.json"
            getInfo "$response"
        fi
    done
}
opciones=$(getopt -o i:n:h --long help,id:,nombre: -- "$@")

if [ "$?" != "0" ]; then
    echo "Opcion/es incorrectas"
    exit 0;
fi

eval set -- "$opciones"

while true; do
    case "$1" in
        -i|--id)
            ids="$2"
            shift 2
            ;;
        -n|--nombre)
            names="$2"
            shift 2
            ;;
        -h|--help)
            mostrarAyuda
            exit 0;
            ;;
         --)
            shift
            break
            ;;
        *)
            echo "ERROR: Argumento desconocido: $1"
            exit 0
            ;;
    esac
done


# Verificar si se proporcionaron argumentos
if [[ -z "$ids" && -z "$names" ]]; then
    echo "Error: Debes proporcionar al menos un argumento." >&2
    exit 1
fi

# Verificar si se proporcionaron IDs y procesarlos
if [ -n "$ids" ]; then

    PersonajeId "$ids"

fi

# Verificar si se proporcionaron nombres y procesarlos
if [ -n "$names" ]; then

    PersonajeName "$names"
fi
