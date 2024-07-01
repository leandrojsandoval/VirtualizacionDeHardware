#########################################################
#               Virtualizacion de Hardware              #
#                                                       #
#   APL1 - Ejercicio 3                                  #
#   Nombre del script: Ejercicio3.sh                    #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Tigani, Martin Sebastian            32788835    #
#       Villegas, Lucas Ezequiel            37792844    #
#       Vivas, Pablo Ezequiel               38703964    #
#                                                       #
#   Instancia de entrega: Reentrega                     #
#                                                       #
#########################################################

function ayuda(){
    echo "USO: $0 [-d|--directorio <directorio_entrada_archivos>] [-x|--extension <extension_archivo_a_analizar>] [-s|--separador <caracter_separador_palabras>] [-o|--omitir <caracter_omitir_palabras>]";
    echo "DESCRIPCIÓN: Este script recibe archivos y devuelve la cantidad de palabras de cada longitur, la cant de palabras por archivo y la o las palabras con mas caracteres.";
}

# ================================= Controles =================================
separador=" "

opciones=$(getopt -o d:x:s:o:h --l directorio:,extension:,separador:,omitir:,help -- "$@" 2> /dev/null);

if [ "$?" != "0" ]; then
    echo "Opcion/es incorrectas";
    exit 0;
fi

eval set -- "$opciones";

while true; do
    case "$1" in
        -d | --directorio)
            directorio="$2";
            shift 2;
            ;;
        -x | --extension)
            extension="$2";
            shift 2;
            ;;
        -s | --separador)
            separador="$2";
            shift 2;
            ;;
        -o | --omitir)
            omitir="$2";
            shift 2;
            ;;
        --)
            shift;
            break;
            ;;
        -h | --help)
            ayuda;
            exit 0;
            ;;
        *)
            echo "ERROR: Argumento desconocido: $1";
            exit 0;
            ;;
    esac
done

# Verificar que se haya proporcionado el directorio de entrada correctamente
if ! [ -d "$directorio" ]; then
    echo "ERROR: Se debe especificar un directorio válido que contenga los archivos.";
    ayuda;
    exit 0;
fi

archivos=$(ls "$directorio"/*"$extension" 2> /dev/null);

# Verificar que el directorio no esté vacío
if [ -z "$archivos" ]; then
    echo "ERROR: El directorio está vacío o no contiene archivos con la extensión especificada.";
    ayuda;
    exit 0;
fi

awk -v separador="$separador" -v omitir="$omitir" -f script.awk $archivos;
