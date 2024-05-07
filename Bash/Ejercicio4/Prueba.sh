#!/bin/bash
archivoMover=$(realpath "$1")
fecha=$(date "+%Y%m%d-%H%M%S")
zip="/mnt/c/Users/pc/desktop/GitPibes/VirtualizacionDeHardware-UNLaM/bash/Ejercicio04${fecha}.tar.gz"


patron="hola"

#linea_coincidente=$(grep -n "$patron" "$archivoMover")

#grep --color=always -n "$patron" "$archivoMover"
#$linea_coincidente=$(echo "grep --color=always -n "$patron" "$archivoMover"")
linea_coincidente=$(grep -n "$patron" "$archivoMover")
echo "$linea_coincidente"

if [ -n "$linea_coincidente" ]; then
    if [ ! -f "$zip" ]; then
        tar -cvzf "$zip" "$archivoMover" >/dev/null
    else
        tar -rvzf "$zip" "$archivoMover" >/dev/null
    fi
else					
	echo "No hubo coindidencia alguna con el patrón." >> "$pathlog"
fi

rm "$archivoMover"
echo "Archivo movido a la papelera: $zip"
