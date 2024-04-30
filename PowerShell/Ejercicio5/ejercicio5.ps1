# Función para imprimir la información de un personaje
param(
    [string[]]$id
)

function Print-CharacterInfo {
    param (
        [string]$Response
    )


    $length = ($Response | ConvertFrom-Json).Count

    if ($length -eq 0) {
        Write-Host "Error: No se encontraron resultados para el nombre/nombres proporcionados."
        exit 1
    }

    if ($length -eq 1) {
        Write-Host "entre en 1"
        ($Response | ConvertFrom-Json) | ForEach-Object {
            Write-Output "Character info:`nId: $($_.id)`nName: $($_.name)`nStatus: $($_.status)`nSpecies: $($_.species)`nGender: $($_.gender)`nOrigin: $($_.origin.name)`nLocation: $($_.location.name)"
        }
    }
    else {
        ($Response | ConvertFrom-Json) | ForEach-Object {
            Write-Output "Character info:`nId: $($_.id)`nName: $($_.name)`nStatus: $($_.status)`nSpecies: $($_.species)`nGender: $($_.gender)`nOrigin: $($_.origin.name)`nLocation: $($_.location.name)"
        }
    }
}

# Función para realizar la búsqueda de personajes por ID
function Get-CharactersById {

    $params = $id -join ','

    $filename = "character_$params.json"

    if (Test-Path $filename) {
        $response = Get-Content $filename -Raw
        Print-CharacterInfo $response
        return 
    }

    $url = "https://rickandmortyapi.com/api/character/$params"
    $response = Invoke-WebRequest $url


    if ($response.Count -eq 0) {
        Write-Host "Error: No se encontraron personajes con los IDs proporcionados."
        exit 1
    }
 
    $response | Set-Content $filename
    Print-CharacterInfo ($response )
}

# Función para realizar la búsqueda de personajes por nombre
function Get-CharactersByName {
    param (
        [string]$Names
    )

    $nameArray = $Names -split ','

    foreach ($name in $nameArray) {
        $filename = "characters_$name.json"

        if (Test-Path $filename) {
            $response = Get-Content $filename -Raw
            Write-Host "cache by name"
            Print-CharacterInfo $response
        }

        $url = "https://rickandmortyapi.com/api/character/?name=$name"
        $response = Invoke-WebRequest $url | Select-Object -ExpandProperty results

        if ($response.Count -eq 0) {
            Write-Host "Error: No se encontraron resultados para el nombre '$name'."
        }
        else {
            $response | ConvertTo-Json | Set-Content $filename
            Print-CharacterInfo ($response | ConvertTo-Json)
        }
    }
}


# Verificar si se proporcionaron argumentos
if (-not ([string]::IsNullOrEmpty($Ids)) -and -not ([string]::IsNullOrEmpty($Names))) {
    Write-Host "Error: No se pueden proporcionar argumentos -i y -n al mismo tiempo." -ForegroundColor Red
    exit 1
}

# Verificar si se proporcionaron IDs y procesarlos

 Get-CharactersById $id


# Verificar si se proporcionaron nombres y procesarlos
if ([string]::IsNullOrEmpty($Names) -eq $false) {
    Get-CharactersByName $Names
}
