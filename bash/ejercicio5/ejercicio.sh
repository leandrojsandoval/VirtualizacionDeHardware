# Función para imprimir la información de un personaje
print_character_info() {
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



# Función para verificar si un personaje con un ID específico está en caché
character_in_cache_by_id() {
    local id=$1
    local filename="character_$id.json"
    if [[ -f "$filename" ]]; then
        local response=$(cat "$filename")
        print_character_info "$response"
    fi
}

# Función para verificar si un personaje con un nombre específico está en caché
character_in_cache_by_name() {
    local name=$1
    local filename="characters_$name.json"
    if [[ -f "$filename" ]]; then
        local response=$(cat "$filename")
        echo "cache by name"
        print_character_info "$response"
    fi
}

# Función para realizar la búsqueda HTTP de personajes por ID
get_characters_by_id() {
    local ids=$1

    character_in_cache_by_id $ids
    local url="https://rickandmortyapi.com/api/character/$ids"
    local response=$(wget -qO- "$url")

    if [[ $(echo "$response" | jq '. | length') -eq 0 ]]; then
        echo "Error: No se encontraron personajes con los IDs proporcionados."
        exit 1
    fi
     echo "$response" > "character_$ids.json"
    print_character_info "$response"
}

# Función para realizar la búsqueda HTTP de personajes por nombre
get_characters_by_name() {
    local names=$1
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
            echo "$response"> "characters_$name.json"
            print_character_info "$response"
        fi
    done

# Analizar los argumentos de línea de comandos
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--id)
            ids="$2"
            shift 2
            ;;
        -n|--nombre)
            names="$2"
            shift 2
            ;;
        *)
            echo "Error: Opción inválida $1" >&2
            exit 1
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
    get_characters_by_id "$ids"
fi

# Verificar si se proporcionaron nombres y procesarlos
if [ -n "$names" ]; then
    get_characters_by_name "$names"
fi
