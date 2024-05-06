# =========================== Encabezado =======================

# Nombre del script: ejercicio04.ps1
# Número de ejercicio: 4
# Trabajo Práctico: 1
# Entrega: Primera entrega

# ==============================================================

# ------------------------ Integrantes ------------------------
# 
#	Nombre				|	Apellido			|	DNI
#	Lucas				|	Villegas			|	37.792.844
# -------------------------------------------------------------

<#
.SYNOPSIS
El script se encarga de monitorizar un directorio enviado por parametro

.PARAMETER directorio
Indica el directorio a monitorear.    

.PARAMETER salida
Indica el directorio donde se hallaran los informes.

.PARAMETER patron
Indica el patron a aplicar en caso de que se modifique un archivo

.PARAMETER kill
Flag en el cual indicamos que debemos eliminar el proceso de un directorio determinado.

.DESCRIPTION
    Este script, se encarga de monitorear un directorio e informar si hubo creación o modificación de un archivo dentro de este
    en caso de modificación, se buscara por medio de un patrón alguna coincidencia en dicho archivo modificado.
    se dejen ejecutas dichas acciones pasadas por parametro.
    El script se invoca de la siguiente forma:
    ./ejercicio04.ps1 -directorio <directorio a monitorear> -salida <directorio donde generar informes> -patron <patron de busqueda>

.EXAMPLE

.\ejercicio04.ps1 -directorio .\monitor -salida .\salida -patron "patron"  
.EXAMPLE

.\ejercicio04.ps1 -directorio ".\monitor" -kill
.EXAMPLE

Get-Help .\ejercicio04.ps1 -Detailed
.EXAMPLE

#>

Param(
    [Parameter(Mandatory=$true)] [ValidateNotNullOrEmpty()] [String] $directorio,
    [Parameter(Mandatory=$false)] [ValidateNotNullOrEmpty()] [String]$salida,
    [Parameter(Mandatory=$false)] [String] $patron, #la validación de este parametro la haremos únicamente en caso de tener que publicar algo.
    [Switch] $kill
)

function Get-validardir() {
    param(
        [Parameter(ValueFromPipeline=$true, ValueFromPipelineByPropertyName=$true)]
        [string]$dir
    )
    
    Begin{
       $resultado=0
    }
    Process{
        $var=Test-Path -Path "$dir" #valida si existe el directorio devuelve "True" si existe y "False" si no existe
        if($var -eq "True"){
           $resultado=1
        }
    }
    End{
        if($resultado -eq 0){
            Write-Output "Directorio inválido"
            exit 1
        }
    }
}

function existe() {
    Param(
        [string] $Nom
    )
    $proceso = get-job -erroraction 'silentlycontinue' | select-object Name | Where-Object { $_.Name -match "$Nom" }

    if ($NULL -ne $proceso -and $proceso.count -ge 0) {
        Write-Host "El directorio ya esta siendo monitorizado"
        exit 1
    }
}

function noExiste() {
        Param(
        [string] $Nom
    )
    $proceso = (get-job -erroraction 'silentlycontinue' | select-object Name | Where-Object { $_.Name -match "$Nom" })
    if ($NULL -eq $proceso) {
        Write-Host "No existe proceso monitorizando el directorio"
        exit 1
    }
}

function Global:moverAzip {
    Param(
        [string] $archivoAmover,
        [string] $archivoZip,
        [string] $log
    )

    if (!(Test-Path "$archivoAmover")) {
        Add-Content "$log" "$archivoAmover es inválido."
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
}

function Global:Monitorear($FullPath,$accion,$Fecha) {
    $var2=Test-Path -Path "$FullPath" -PathType Leaf -ErrorAction Ignore
    if($var2 -eq $true){
        #establecer fecha
        $fechaMonitoreo=Get-Date -Format "yyyyMMdd-HHmmss"
        #crear archivo log
        $log="$SAL\$fechaMonitoreo.txt"
        New-Item -ItemType File -Path "$log"
        if($accion -eq "CHANGED"){
            #Añadimos el reporte al archivo log
            Add-Content "$log" "$Fecha $FullPath ha sido modificado"
            #buscamos patron en archivo
            $matcheo=Select-String -Path "$FullPath" -Pattern $Patron
            if($matcheo){
                #añadiendo contenido dentro de un archivo
                Add-Content ".\Patron$fechaMonitoreo.txt" "$matcheo"
                #generar ruta zip
                $ruta_zip="$SAL\$fechaMonitoreo.zip"
            
                Global:moverAzip $FullPath $ruta_zip $log
            }
            else{Add-Content "$log" "$Fecha $FullPath no hubo coincidencia alguna."}
        }
        if($accion -eq "RENAMED"){
           Add-Content "$log" "$Fecha $FullPath ha sido renombrado"
        }
        if($accion -eq "CREATED"){
           Add-Content "$log" "$Fecha $FullPath ha sido creado"
        }
    }
}

if ($kill) {
    $directorioAEliminar=split-path -leaf "$directorio"
    $cadena=$directorioAEliminar.Replace('\','')
    noExiste -Nom $cadena
    Get-Job -erroraction 'silentlycontinue' | Select-Object Name | Where-Object { $_.Name -match "$directorioAEliminar" } | ForEach-Object { remove-job -force -Name $_.Name }
    if (Test-Path -Path "./$directorioAEliminar.txt" -PathType Leaf) {
        get-content -path "./$directorioAEliminar.txt"
        Remove-Item -Path "./$directorioAEliminar.txt"
    }
    else {
        write-host "El Directorio $directorioAEliminar no ha sufrido cambios."
    }
    Write-Host "El proceso ha sido finalizado."
    exit 1;
}


$Global:PAT=(Resolve-Path -LiteralPath "$directorio").ToString()
$Global:nombre = split-path -leaf "$directorio"
Write-Output "$PAT"
Get-validardir "$PAT"

$cadena=$PAT.Replace('\','')

existe -Nom $cadena


$Global:SAL=(Resolve-Path -LiteralPath "$salida").ToString()
Get-validardir "$SAL"
Write-Output "$SAL"
$global:Patron = "$patron"


$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = $PAT
$watcher.Filter = "*.*"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

$action = {
        $details = $event.SourceEventArgs
        $Name = $details.Name
        $FullPath = $details.FullPath
        $OldFullPath = $details.OldFullPath
        $OldName = $details.OldName
        # tipo de cambio:
        $ChangeType = $details.ChangeType

        # cuándo ocurrió el cambio:
        $Timestamp = $event.TimeGenerated

        $global:all = $details
        $ev = ""
        switch ($ChangeType)
        {
            "Changed"  { $ev="CHANGED" }
            "Created"  { $ev="CREATED" }
            "Renamed"  { $ev="RENAMED" }  
            # cualquier superficie de tipo de cambio no controlada aquí:
            default   {}
        }
        $FullPath = $all.FullPath
        $Timestamp = $event.TimeGenerated
        
        Monitorear "$FullPath" $ev $Timestamp
}

Register-ObjectEvent -InputObject $watcher -EventName Created -Action $action -SourceIdentifier "$cadena-Created" | out-null
Register-ObjectEvent -InputObject $watcher -EventName Changed -Action $action -SourceIdentifier "$cadena-Changed" | out-null
Register-ObjectEvent -InputObject $watcher -EventName Renamed -Action $action -SourceIdentifier "$cadena-Renamed" | out-null