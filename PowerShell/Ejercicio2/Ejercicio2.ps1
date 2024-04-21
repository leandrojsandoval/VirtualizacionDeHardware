#########################################################
#               Virtualizacion de hardware              #
#   APL1 - Ejercicio 2                                  #
#   Nombre del script: Ejercicio2.ps1                   #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#                                                       #
#	Instancia de entrega: Primera Entrega               #
#                                                       #
#########################################################

Param(
    [Parameter(Mandatory=$true)] [string]$matriz1,
    [Parameter(Mandatory=$true)] [string]$matriz2,
    [Parameter(Mandatory=$false)] [string]$separador = ","
);

# cargarMatrizDesdeRuta: Para cargar la matriz desde el archivo utilizo un hash table que tenga como claves filas y columnas.
# Realizo un split para separar los elementos con ayuda del separador y voy insertando los elementos.
function cargarMatrizDesdeRuta { 
    Param ( [Parameter(Mandatory=$true)] [string]$rutaArchivoMatriz, [Parameter(Mandatory=$true)] [string]$separador);
    $matriz = @{ };
    $i = 0;
    foreach ($registro in Get-Content -Path $rutaArchivoMatriz) {
        $elementosPorRegistro = $registro -split $separador;
        for ($j = 0; $j -lt $elementosPorRegistro.Length; $j++) {
            $matriz["$i,$j"] = [int]$elementosPorRegistro[$j];
        }
        $i++;
    }
    return $matriz;
}

# calcularCantidadDeColumnasDeMatriz y calcularCantidadDeFilasDeMatriz: Estas funciones se aplican a la hash table directamente.
function calcularCantidadDeColumnasDeMatriz {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz);
    $cantidadColumnas = 0;
    foreach ($key in $matriz.Keys) {
        $columnaActual = $key.Split(',')[1];
        if ($columnaActual -gt $cantidadColumnas) {
            $cantidadColumnas = [int]$columnaActual;
        }
    }
    return $cantidadColumnas + 1;
}

function calcularCantidadDeFilasDeMatriz {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz);
    $cantidadFilas = 0;
    foreach ($key in $matriz.Keys) {
        $filaActual = $key.Split(',')[0];
        if ($filaActual -gt $cantidadFilas) {
            $cantidadFilas = [int]$filaActual;
        }
    }
    return $cantidadFilas + 1;
}

# esMatrizColumna: Si es una matriz columna, quiere decir que tendra una unica columna
function esMatrizColumna {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    return (calcularCantidadDeColumnasDeMatriz $matriz) -eq 1;
}

# esMatrizColumna: Si es una matriz cuadrada, quiere decir que la cantidad de columnas son iguales que la cantidad de filas
function esMatrizCuadrada {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    return (calcularCantidadDeFilasDeMatriz $matriz) -eq (calcularCantidadDeColumnasDeMatriz $matriz);
}

# esMatrizFila: Si es una matriz fila, quiere decir tendra una unica fila
function esMatrizFila {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    return (calcularCantidadDeFilasDeMatriz $matriz) -eq 1;
}

# mostrarMatriz: Reconstruyo la matriz que se encuentra en la hash table colocando nuevamente el separador para que tenga
# el mismo formato que los mismo archivos
function mostrarMatriz {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz, [Parameter(Mandatory=$true)] [string]$separador );
    
    $filas = calcularCantidadDeFilasDeMatriz -matriz $matriz;
    $columnas = calcularCantidadDeColumnasDeMatriz -matriz $matriz;

    $matrizFormateada = @();

    for ($i = 0; $i -lt $filas; $i++) {
        $filaActual = @();
        for ($j = 0; $j -lt $columnas; $j++) {
            $filaActual += $matriz["$i,$j"];
        }
        $matrizFormateada += $filaActual -join $separador;
    }

    $matrizFormateada;
}

# multiplicarMatrices: Es la misma logica que la miltiplicacion de matrices en C
function multiplicarMatrices {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz1, [Parameter(Mandatory=$true)] [hashtable]$matriz2 );

    $filasMatriz1 = calcularCantidadDeFilasDeMatriz -matriz $matriz1;
    $columnasMatriz1 = calcularCantidadDeColumnasDeMatriz -matriz $matriz1;
    $columnasMatriz2 = calcularCantidadDeColumnasDeMatriz -matriz $matriz2;

    $matrizResultado = @{};

    for($i = 0; $i -lt $filasMatriz1; $i++) {
        for($j = 0; $j -lt $columnasMatriz2; $j++) {
            $suma = 0;
            for($k = 0; $k -lt $columnasMatriz1; $k++) {
                $suma += $matriz1["$i,$k"] * $matriz2["$k,$j"];
            }
            $matrizResultado["$i,$j"] = $suma;
        }
    }

    return $matrizResultado;
}

# obtenerCantidadDeColumnasDeMatriz y obtenerCantidadDeFilasDeMatriz: Estas se aplican unicamente a las matrices directamente
# desde los archivos, es la diferencia con las funciones calcularCantidadDeColumnasDeMatriz y calcularCantidadDeFilasDeMatriz
function obtenerCantidadDeColumnasDeMatriz {
    Param ( [string]$rutaArchivoMatriz);
    $primeraLinea = Get-Content -Path $rutaArchivoMatriz -TotalCount 1;
    return $primeraLinea.Split($separador).Count;
}

function obtenerCantidadDeFilasDeMatriz {
    Param ( [string]$rutaArchivoMatriz);
    return (Get-Content -Path $rutaArchivoMatriz).Count;
}

# verificarMatriz: Verifica que la matriz que venga en el archivo corresponda a una matriz completa.
# Es decir, por ejemplo, que si existe una matriz:
# 1,2,3
# 1
# 4,5
# Se dara como invalida, esto se controla calculando la cantidad de elementos por linea de registros
function verificarMatriz {
    Param ( [string]$rutaArchivoMatriz, [string]$separador);

    $contenidoArchivo = Get-Content -Path $rutaArchivoMatriz;
    $cantidadDeColumnasPrimeraLinea = ($contenidoArchivo[0] -split $separador).Count;
    $matrizVerificada = $true;

    for ($i = 0; $i -lt $contenidoArchivo.Length -and $matrizVerificada; $i++) {
        $cantidadDeColumnasLineaActual = ($contenidoArchivo[$i] -split $separador).Count;
        if($cantidadDeColumnasPrimeraLinea -ne $cantidadDeColumnasLineaActual) {
            $matrizVerificada = $false;
        }
    }
    return $matrizVerificada;
}

# verificarSeparador: Verifica que el separador no sea un digito, un menos (-) ni tampoco que sea mayor a un caracter
function verificarSeparador {
    Param ( [string]$separador );
    return -not (($separador.Length -gt 1) -or ($separador -match '\d') -or ($separador -eq '-'));
}

# verificarSeparadorConSeparadorEnArchivo: Los separadores deben ser iguales tanto el de los archivos como el proporcionado
function verificarSeparadorConSeparadorEnArchivo {
    Param ( [string]$rutaArchivoMatriz, [string]$separador);
    $contenidoArchivo = Get-Content -Path $rutaArchivoMatriz;
    $mismoSeparador = $true;
    foreach($linea in $contenidoArchivo) {
        $separadoresPorLinea = $linea -replace "[\d-]", "";
        for ($i = 0; $i -lt $separadoresPorLinea.Length -and $mismoSeparador; $i++) {
            if (-not ($separadoresPorLinea[$i] -contains $separador)) {
                $mismoSeparador = $false;
            }
        }
    }
    return $mismoSeparador;
}

function main {

    # Verifico que el directorio proporcionado de ambos archivos sea correcto
    if(-not (Test-Path -Path $matriz1) -or -not (Test-Path -Path $matriz2)) {
        Write-Error "ERROR: Alguno de los directorios pasados por párametro no corresponden a un directorio valido";
        exit;
    }

    # Verifico que los archivos no esten vacios
    if ((Get-Content -Path $matriz1).Length -eq 0 -or (Get-Content -Path $matriz2).Length -eq 0) {
        Write-Error "ERROR: Alguno de los archivos pasados por parámetro está vacío.";
        exit;
    }    

    # Verifico que el separador proporcionado sea un separador valido (no números y tampoco el símbolo menos)
    if(-not (verificarSeparador $separador)) {
        Write-Error "ERROR: El separador indicando no es un separador valido";
        exit;
    }

    # Verifico que el separador coincida con el separador de los archivos
    if(-not(verificarSeparadorConSeparadorEnArchivo $matriz1 $separador) -or  -not(verificarSeparadorConSeparadorEnArchivo $matriz2 $separador)) {
        Write-Error "ERROR: El separador proporcionado no coincide con el separador de los archivos";
        exit;
    }
    
    # Verifico que el archivo tenga el formato de una matriz
    if(-not (verificarMatriz $matriz1 $separador) -or -not (verificarMatriz $matriz2 $separador)) {
        Write-Error "ERROR: Los archivos en donde se encuentran las matrices no coinciden con un formato válido";
        exit;
    }

    # Verifico la condicion para multiplicacion de matrices (columnas de m1 = filas de m2)
    if((obtenerCantidadDeColumnasDeMatriz $matriz1) -ne (obtenerCantidadDeFilasDeMatriz $matriz2)) {
        Write-Error "ERROR: No se pueden multiplicar las matrices ya que el número de columnas de la primera matriz no coincide con el número de filas de la segunda matriz."
        exit;
    }

    $m1 = cargarMatrizDesdeRuta -rutaArchivoMatriz $matriz1 -separador $separador;

    $m2 = cargarMatrizDesdeRuta -rutaArchivoMatriz $matriz2 -separador $separador;

    Write-Output "Matriz 1: "
    mostrarMatriz $m1 $separador;

    Write-Output "Matriz 2: "
    mostrarMatriz $m2 $separador;

    $mResultado = multiplicarMatrices -matriz1 $m1 -matriz2 $m2;

    Write-Output "Matriz Resultado: "
    mostrarMatriz $mResultado $separador;

    Write-Output "El orden de la matriz es de $(calcularCantidadDeFilasDeMatriz $mResultado)X$(calcularCantidadDeColumnasDeMatriz $mResultado)";

    if(esMatrizCuadrada $mResultado) {
        Write-Output "La matriz resultado es una matriz cuadrada";
    } else {
        Write-Output "La matriz resultado no es una matriz cuadrada";
    }

    if(esMatrizFila $mResultado) {
        Write-Output "La matriz es una matriz fila";
    } else {
        Write-Output "La matriz no es una matriz fila";
    }

    if(esMatrizColumna $mResultado) {
        Write-Output "La matriz es una matriz columna";
    } else {
        Write-Output "La matriz no es una matriz columna";
    }
}

main;