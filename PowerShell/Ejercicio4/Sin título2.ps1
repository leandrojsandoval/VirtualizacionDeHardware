# =========================== Encabezado =======================

# Nombre del script: Ejercicio06.ps1
# Número de ejercicio: 6
# Trabajo Práctico: 2
# Entrega: Cuarta entrega

# ==============================================================

# ------------------------ Integrantes ------------------------
# 
#	Nombre				|	Apellido			|	DNI
#	Matías				|	Beltramone			|	40.306.191
#	Eduardo				|	Couzo Wetzel		|	43.584.741
#	Brian				|	Menchaca			|	40.476.567
#	Ivana				|	Ruiz				|	33.329.371
#	Lucas				|	Villegas			|	37.792.844
# -------------------------------------------------------------

<#
.SYNOPSIS
Este script simula una papelera de reciclaje
.DESCRIPTION
Este script simula una papelera de reciclaje
al borrar un archivo se tiene la posibilidad
de recuperarlo en un futuro.
La papelera de reciclaje será un archivo zip
y la misma se guarda en el home del usuario
que ejecuta el script.
.PARAMETER listar
 Lista el contenido de la papelera.zip
.PARAMETER vaciar 
 Deja vacia la papelera.zip
.PARAMETER eliminar 
 Elimina un archivo y lo envia a la papelera.zip
.PARAMETER recuperar 
 recupera un archivo de la papelera.zip
.EXAMPLE
Listar contenido de papelera:
  ./Ejercicio06.ps1 -listar
.EXAMPLE
Vaciar contenido de papelera:
  ./Ejercicio06.ps1 -vaciar  
.EXAMPLE
Recuperar archivo de papelera:  
  ./Ejercicio06.ps1 -recuperar archivo
.EXAMPLE
Eliminar archivo (Se envía a la papelera):
  ./Ejercicio06.ps1 -eliminar archivo 
.EXAMPLE
   ./Ejercicio06.ps1 -borrar archivo
#>

param(
  [Parameter(Mandatory=$false)]
  [String]
  $archivo
)

function moverAzip{
    Param(
        [string] $archivoAmover,
        [string] $archivoZip
    )

  if(!(Test-Path "$archivoAmover")){
    Write-Host "Parámetro archivo en función eliminar no es válido"
    Write-Host "Por favor consulte la ayuda"
    exit 1
  }
  $archivoMoverRutaAbs=$(Resolve-Path "$archivoAmover");

  #si no existe el archivo zip, lo crea.
  if(!(Test-Path "$archivoZip")){
    $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "create");
    $zip.Dispose();
  }

  $compressionLevel = [System.IO.Compression.CompressionLevel]::Fastest;
  $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "update");
  [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, "$archivoMoverRutaAbs", "$archivoMoverRutaAbs", $compressionLevel);
  $zip.Dispose();

  Remove-Item "$archivoMoverRutaAbs"
  Write-Host "Archivo movido"
}

function moverAzip2 {
    Param(
        [string] $archivoAmover,
        [string] $archivoZip
    )

    if (!(Test-Path "$archivoAmover")) {
        Write-Host "Parámetro archivo en función eliminar no es válido"
        Write-Host "Por favor consulte la ayuda"
        exit 1
    }

    $archivoMoverNombre = [System.IO.Path]::GetFileName($archivoAmover)
    $archivoMoverRutaAbs = $(Resolve-Path "$archivoAmover")

    # Si no existe el archivo zip, lo crea.
    if (!(Test-Path "$archivoZip")) {
        $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "create")
        $zip.Dispose()
    }

    $compressionLevel = [System.IO.Compression.CompressionLevel]::Fastest
    $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "update")
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, "$archivoMoverRutaAbs", "$archivoMoverNombre", $compressionLevel)
    $zip.Dispose()

    Remove-Item "$archivoAmover"
    Write-Host "Archivo movido"
}


$fechaMonitoreo=Get-Date -Format "yyyyMMdd-HHmmss"
Write-Output $fechaMonitoreo

$rutazip = "C:\Users\pc\Desktop\GitPibes\VirtualizacionDeHardware-UNLaM\Powershell\Ejercicio4\salida\$fechaMonitoreo.zip"
Write-Output $rutazip

moverAzip2 $archivo $rutazip

#eliminar $archivo
