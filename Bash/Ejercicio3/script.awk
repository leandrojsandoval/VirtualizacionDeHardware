BEGIN {
    cantidadDePalabrasTotales = 0;
    palabraRepetidaMasVeces = 0;
    palabrasOmitidasPorArchivo = 0;
    palMAxRepet = "";
    FS = separador;
    # Los caracteres que llegan para omitir, los spliteo armando el array de caracteres
    split(omitir, caracteresOmision, "");
}

{
	cantidadDePalabrasTotales += NF;
    
    for(i = 1; i <= NF; i++){

        # Elimino caracteres de puntuación y de salto de línea
        gsub("\\.|,|\r|\n", "", $i);

		longitudPalabra = length($i);

        if (longitudPalabra > 0) {

            # Bandera para verificar si lo tengo que omitir o no
            incluirPalabra = 1;

            # Lo busco en el array de caracteres omitidos si la palabra contiene esa letra
            for (j = 1; j <= length(caracteresOmision); j++) {
                if (index($i, caracteresOmision[j]) > 0) {
                    incluirPalabra = 0;
                    palabrasOmitidasPorArchivo++;
                    break;
                }
            }

            # Si lo tengo que incluir, lo agrego al array e incremento la cantidad de ocurrencias
            if (incluirPalabra) {
                cantidadPorLongitud[longitudPalabra]++;
                palabras[$i]++;
            }
        }

	}

    # Recuento de caracteres
    for (i = 1; i <= length($0); i++) {
        if(substr($0, i, 1) != " "){
            contadorDeCaracteres[substr($0, i, 1)]++;
        }
    }

}

END {

    printf("La cantidad de palabras totales en el texto son: %d\n", cantidadDePalabrasTotales);
    printf("La cantidad de palabras omitidas en el texto son: %d\n", palabrasOmitidasPorArchivo);
    printf("Palabras validas a contabilidar: %d\n", cantidadDePalabrasTotales - palabrasOmitidasPorArchivo);

    for(cantidadCaracteres in cantidadPorLongitud) {
		printf("Palabras con longitud (%d): => %d \n",cantidadCaracteres, cantidadPorLongitud[cantidadCaracteres]);
	}

    printf("\n");

    for(clave in palabras) {
        if(palabras[clave] > palabraRepetidaMasVeces){
            palabraRepetidaMasVeces = palabras[clave];
            palMAxRepet = clave;
        }
    }

    printf("Las palabras mas repetidas son: \n");
    for(palabra in palabras) {
        if(palabraRepetidaMasVeces == palabras[palabra]){
            printf("\t%s\n", palabra);
        }
    }
    
	promedioPalabrasPorArchivo = (cantidadDePalabrasTotales - palabrasOmitidasPorArchivo) / (ARGC -1);
	printf("El promedio de palabras por archivo es: %d\n", promedioPalabrasPorArchivo);
    caracterMasRepetido = "";
    ocurrenciasCaracterMasRepetido = 0;

    for (caracter in contadorDeCaracteres) {
        if (contadorDeCaracteres[caracter] > ocurrenciasCaracterMasRepetido) {
            caracterMasRepetido = caracter;
            ocurrenciasCaracterMasRepetido = contadorDeCaracteres[caracter];
        }
    }

    print "El caracter más repetido es '" caracterMasRepetido "' con " ocurrenciasCaracterMasRepetido " ocurrencias.";

}
