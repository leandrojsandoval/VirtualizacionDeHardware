#########################################################
#			    Virtualizacion de hardware		        #
#	APL1 - Ejercicio 2		                            #
#	Nombre del script: Ejercicio2.ps1	                #
#							                            #
#	Integrantes:		                                #
#         		                                        #
#       Ocampo, Nicole                      44451238	#
#       Sandoval Vasquez, Juan Leandro      41548235    #
#							                            #
#	Instancia de entrega:  Entrega		                #
#							                            #
#########################################################

Param(
    [Parameter(Mandatory=$true)] [string]$matriz1,
    [Parameter(Mandatory=$true)] [string]$matriz2,
    [Parameter(Mandatory=$false)] [string]$separador = ","
);

function verificarMatriz {
    Param ( [string]$rutaArchivoMatriz, [string]$separador);

    $contenidoArchivo = Get-Content -Path $rutaArchivoMatriz;

    $cantidadDeColumnasPrimeraLinea = $contenidoArchivo[0].Split($separador).Count;
    for ($i = 1; $i -lt $contenidoArchivo.Length; $i++) {
        $cantidadDeColumnasLineaActual = $contenidoArchivo[$i].Split($separador).Count;
        if($cantidadDeColumnasPrimeraLinea -ne $cantidadDeColumnasLineaActual) {
            return $false;
        }
    }

    return $true;
}

function obtenerCantidadDeColumnasDeMatriz {
    Param ( [string]$rutaArchivoMatriz, [string]$separador);
    $primeraLinea = Get-Content -Path $rutaArchivoMatriz -TotalCount 1;
    $cantidadDeColumnas = $primeraLinea.Split($separador).Count;
    return $cantidadDeColumnas;
}

function calcularCantidadDeColumnasDeMatriz {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    $maxColumna = 0;
    foreach ($key in $matriz.Keys) {
        $columna = $key.Split(',')[1];
        if ($columna -gt $maxColumna) {
            $maxColumna = [int]$columna;
        }
    }
    return $maxColumna + 1;
}

function obtenerCantidadDeFilasDeMatriz {
    Param ( [string]$rutaArchivoMatriz);
    $cantidadDeFilas = (Get-Content -Path $rutaArchivoMatriz).Count
    return $cantidadDeFilas;
}

function calcularCantidadDeFilasDeMatriz {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    $maxFila = 0;
    foreach ($key in $matriz.Keys) {
        $fila = $key.Split(',')[0];
        if ($fila -gt $maxFila) {
            $maxFila = [int]$fila;
        }
    }
    return $maxFila + 1;
}

function convertirFilaStringAInt { 
    Param ( 
        [Parameter(Mandatory=$true)] [string]$linea, 
        [Parameter(Mandatory=$true)] [string]$separador
    );

    $fila = $linea.Split($separador);
    $filaEntera = @();

    foreach ($elemento in $fila) {
        $filaEntera += [int]$elemento
    }

    return $filaEntera;
}

function cargarMatrizDesdeRuta { 
    Param ( [Parameter(Mandatory=$true)] [string]$rutaArchivo, [Parameter(Mandatory=$true)] [string]$separador);

    $matriz = @{ };

    $columnas = obtenerCantidadDeColumnasDeMatriz -rutaArchivoMatriz $rutaArchivo -separador $separador;

    $i = 0;

    foreach ($registro in Get-Content -Path $rutaArchivo) {
        $elementosPorRegistro = $registro -split $separador;
        for ($j = 0; $j -lt $columnas; $j++) {
            $matriz["$i,$j"] = [int]$elementosPorRegistro[$j];
        }
        $i++;
    }

    return $matriz;
}

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

    $matrizFormateada | ForEach-Object { Write-Host $_ };
}

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

function esMatrizCuadrada {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    if((calcularCantidadDeFilasDeMatriz $matriz) -eq (calcularCantidadDeColumnasDeMatriz $matriz)) {
        return $true;
    }
    return $false;
}

function esMatrizFila {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    if((calcularCantidadDeFilasDeMatriz $matriz) -eq 1) {
        return $true;
    }
    return $false;
}

function esMatrizColumna {
    Param ( [Parameter(Mandatory=$true)] [hashtable]$matriz );
    if((calcularCantidadDeColumnasDeMatriz $matriz) -eq 1) {
        return $true;
    }
    return $false;
}

function main {

    if(-not (verificarMatriz $matriz1 $separador) -or -not (verificarMatriz $matriz2 $separador)) {
        Write-Host "ERROR: Los archivos en donde se encuentran las matrices no coinciden con un formato válido";
        exit;
    }

    if((obtenerCantidadDeColumnasDeMatriz $matriz1 $separador) -ne (obtenerCantidadDeFilasDeMatriz $matriz2)) {
        Write-Error "ERROR: No se pueden multiplicar las matrices ya que el número de columnas de la primera matriz no coincide con el número de filas de la segunda matriz."
        exit;
    }

    $m1 = cargarMatrizDesdeRuta -rutaArchivo $matriz1 -separador $separador;

    $m2 = cargarMatrizDesdeRuta -rutaArchivo $matriz2 -separador $separador;

    $mResultado = multiplicarMatrices -matriz1 $m1 -matriz2 $m2;

    Write-Host "";

    Write-Host "Matriz 1: "
    mostrarMatriz $m1 $separador;

    Write-Host "Matriz 2: "
    mostrarMatriz $m2 $separador;

    Write-Host "Matriz Resultado: "
    mostrarMatriz $mResultado $separador;

    Write-Host "El orden de la matriz es de obtener $(calcularCantidadDeFilasDeMatriz $mResultado)X$(calcularCantidadDeColumnasDeMatriz $mResultado)";

    if(esMatrizCuadrada $mResultado) {
        "La matriz resultado es una matriz cuadrada";
    } else {
        "La matriz resultado no es una matriz cuadrada";
    }

    if(esMatrizFila $mResultado) {
        "La matriz es una matriz fila";
    } else {
        "La matriz no es una matriz fila";
    }

    if(esMatrizColumna $mResultado) {
        "La matriz es una matriz columna";
    } else {
        "La matriz no es una matriz columna";
    }

}

main;