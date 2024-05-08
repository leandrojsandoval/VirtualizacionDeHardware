
Param(
    [Parameter(Mandatory=$true)] [ValidateNotNullOrEmpty()] [String] $archivoMover,
    [Parameter(Mandatory=$false)] [ValidateNotNullOrEmpty()] [String]$zip,
    [Parameter(Mandatory=$false)] [String] $log,
    [Switch] $kill
)


function moverAzip {
    Param(
        [string] $archivoAmover,
        [string] $archivoZip,
        [string] $log
    )
    Write-output "$archivoAmover"
    Write-output "$archivoZip"
    Write-output "$log"

    if (!(Test-Path "$archivoAmover")) {
        Add-Content "$log" "$archivoAmover es inválido."
        exit 1
    }

    $archivoMoverNombre = [System.IO.Path]::GetFileName($archivoAmover)
    $archivoMoverRutaAbs = $(Resolve-Path "$archivoAmover")

    # Si no existe el archivo zip, lo crea.
    if (!(Test-Path "$archivoZip")) {
        $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "create");
        $zip.Dispose();
    }

    $compressionLevel = [System.IO.Compression.CompressionLevel]::Fastest;
    $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "update");
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, "$archivoMoverRutaAbs", "$archivoMoverNombre", $compressionLevel);
    $zip.Dispose();

     Remove-Item "$archivoAmover"
}

function moverAzip2 {
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

function moverAzip3 {
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
        $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "create");
        $zip.Dispose();
    }
    $compressionLevel = [System.IO.Compression.CompressionLevel]::Fastest
    $zip = [System.IO.Compression.ZipFile]::Open("$archivoZip", "update");
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, "$archivoMoverRutaAbs", "$archivoMoverNombre", "Fastest");
    $zip.Dispose();

    Remove-Item "$archivoAmover"
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
moverAzip3 "$archivoMover" "$zip" "$log"

#eliminar "$archivoMover" "$zip" "$log"