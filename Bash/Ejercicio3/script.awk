
BEGIN{
    print("soy el comienzo de el awk");
    cantpalabrasTotales=0
    #cantArchivo=0
    maxRepeticiones=0;
    palMAxRepet="";
    FS = separador;
}

{
        #  if (index($i, caracter_omitir) == 0) {
        #      print NF;
        #      cantpalabrasTotales+=NF;
        #  }
        cantpalabrasTotales+=NF;
		
}
{
	for(i=1; i<=NF;i++){
        gsub("\\.", "", $0);
        gsub(",", "", $0);
        gsub("\r", "", $0);
        gsub("\n", "", $0)
		longPal=length($i);
        #printf("longitd de la palabra es %d\t", longPal)
		cantXLongitud[longPal]++;
	}
}
{
    for(i=1; i<=NF; i++){
        palabras[$i]++;
    }

}
{
     for (i = 1; i <= length($0); i++) {
        if(substr($0, i, 1)!= " "){
            char_count[substr($0, i, 1)]++;
        }
     }

 }


END{
    

    printf("La cantidad de palabras totales en el texto son: %d\n", cantpalabrasTotales);
    for(cantCaracteres in cantXLongitud){
		printf("Palabras con longitud (%d): => %d \n",cantCaracteres, cantXLongitud[cantCaracteres]);
	}
    printf("\n\n");

    # for(clave in palabras){
    #     printf("La palabra (%s) se repitio: %d veces \n",clave, palabras[clave]);
    # }


    for(clave in palabras){
        if(palabras[clave] > maxRepeticiones){
            maxRepeticiones=palabras[clave];
            palMAxRepet=clave;
        }
    }
    printf("Las palabras mas repetidas son: \n");
    for(clave in palabras){
        if(maxRepeticiones==palabras[clave]){
            printf(" %s\n", clave);
        }
    }
    #printf("La palabra mas repetida es: %s con %d repeticiones\n", palMAxRepet, maxRepeticiones);
    print(cantArchivo);
	palPromxArchivo=cantpalabrasTotales/(ARGC -1);
	printf("el promedio de palabras por archivo es: %d\n\n\n", palPromxArchivo);

    max_char = "";
    max_count = 0;
    for (char in char_count) {
        if (char_count[char] > max_count) {
            max_char = char;
            max_count = char_count[char];
        }
    }
    print "El caracter más repetido es '" max_char "' con " max_count " ocurrencias.";

    
}


