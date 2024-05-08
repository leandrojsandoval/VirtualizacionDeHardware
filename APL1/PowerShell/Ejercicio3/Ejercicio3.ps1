#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL1 - Ejercicio 3                                  #
#   Nombre del script: Ejercicio3.ps1                   #
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

<#
.SYNOPSIS
Script para contabilizar palabras y contabilizar caracteres en archivos

.DESCRIPTION
Se recibe uno o varios archivos, con los cuales se calcula:
- El total de palabras que hay entre todos los archivos. 
- El promedio de palabras por archivo
- La cantidad de palabras que hay con 1 caracter, 2 caracter, 3 caracter, etc
- La palabra o palabras que aparece mas veces en todos estos arhivos

.PARAMETER directorio
Ruta del directorio que contiene los archivos a procesar. 

.PARAMETER extension
 Si esta presente indica la extension de el archivo a analizar

.PARAMETER separador
Muestra que caracter quiero utilizar como separador de palabra. Si no pongo nada toma como defecto  el espacio " ".

.PARAMETER omitir
Caracter que quiero que las palabras q lo contengan no sean contabilizadas

.EXAMPLE
.\Ejercicio3.ps1 -directorio "C:\ruta\del\directorio" -extension "archivo1.txt" 

.NOTES
Consideraciones: 
• Se asume que si no ponemos separador se toma como separador el espacio " " 
• Se asume que no hay caracteres a omitir por defecto
#>

Param(
    [Parameter(Mandatory=$true, Position=1)]
    [string]$Directorio,

    [Parameter(Mandatory=$false)]
    [string]$directorio2,

    [Parameter(Mandatory=$false)]
    [string]$extension,

    [Parameter(Mandatory=$false)]
    [char]$separador,

    [Parameter(Mandatory=$false)]
    [char]$omitir
)

# Verificar si el directorio existe
if (!(Test-Path $Directorio -PathType Container)) {
    Write-Host "El directorio especificado no existe.";
    exit;
}

if (-not $separador) {
    $separador = " ";
}

# Conteo global
$conteoGlobal = @{};
$conteoCaracteres = @{};
$conteoPalabras = @{};

# Obtener la lista de archivos de texto en el directorio
$archivos = Get-ChildItem -Path $Directorio -File -Filter "*$extension";

$cantidadArchivos=0;
$cantidadPalTotales=0;

# Iterar sobre cada archivo de texto
foreach ($archivo in $archivos) {

    # Leer el contenido del archivo
    $Contenido = Get-Content $archivo.FullName;
    $cantidadArchivos++;
    $contenido=$contenido -replace '\.', '';
    $palabras=$contenido -split $separador;

    foreach ($palabra in $palabras) {

        $longitud = $palabra.Length

        if ($conteoGlobal.ContainsKey($longitud)) {
            $conteoGlobal[$longitud] += 1;
        } else {
            $conteoGlobal[$longitud] = 1;
        }

        # Contar los caracteres y actualizar el conteo global
        
        $caracteres = $palabra.ToCharArray();
        
        foreach ($caracter in $caracteres) {
            if ($conteoCaracteres.ContainsKey($caracter)) {
                $conteoCaracteres[$caracter] += 1;
            } else {
                $conteoCaracteres[$caracter] = 1;
            }
        }

        # Contar las palabras y actualizar el conteo global
        if ($conteoPalabras.ContainsKey($palabra)) {
            $conteoPalabras[$palabra] += 1;
        } else {
            $conteoPalabras[$palabra] = 1;
        }

    }

    # Contar las palabras en el contenido del archivo
    $NumeroPalabras = ($Contenido -split '\s+').Count;
    $cantidadPalTotales+=$NumeroPalabras;
}

# Mostrar el conteo global de palabras de cada longitud
foreach ($longitud in $conteoGlobal.Keys) {
    $cantidad = $conteoGlobal[$longitud];
    Write-Host "Palabras de longitud $longitud :  esta $cantidad veces";
}

# Encontrar la palabra o palabras más repetidas
$palabrasMasRepetidas = @();
$maxRepeticiones = ($conteoPalabras.GetEnumerator() | Sort-Object -Property Value -Descending)[0].Value;

foreach ($palabra in $conteoPalabras.Keys) {
    if ($conteoPalabras[$palabra] -eq $maxRepeticiones) {
        $palabrasMasRepetidas += $palabra;
    }
}

Write-Host "La palabra o palabras más repetidas son: $($palabrasMasRepetidas -join ', ')"

# Encontrar el carácter más repetido
$caracterMasRepetido = ($conteoCaracteres.GetEnumerator() | Sort-Object -Property Value -Descending)[0].Key;
$cantidadRepeticiones = $conteoCaracteres[$caracterMasRepetido];
Write-Host "El carácter más repetido es '$caracterMasRepetido' con $cantidadRepeticiones repeticiones.";

Write-Host "La cantidad total de palabras de los archivos es: $cantidadPalTotales";
$promPalArch = $cantidadPalTotales / $cantidadArchivos;
Write-Output "El promedio de palabras por archivo es: $promPalArch";
