#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL1 - Ejercicio 5                                  #
#   Nombre del script: Ejercicio5.ps1                   #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Vivas, Pablo Ezequiel               38703964    #
#       Villegas, Lucas Ezequiel            37792844    #
#                                                       #
#   Instancia de entrega: Primera Entrega               #
#                                                       #
#########################################################


<#
.SYNOPSIS
Script que facilite la consulta de informacion relacionada a la serie Rick and
Morty

.DESCRIPTION
El script permitira buscar informacion de los personajes por su id o su nombre a traves de la api
https://rickandmortyapi.com/ y pueden enviarse mas de 1 id o nombre. 

.PARAMETER id
Array de ids. 

.PARAMETER nombre
Array de nombres. 

.EXAMPLE
.\Ejercicio5.ps1 -id 1,2
.\Ejercicio5.ps1 -nombre rick, morty
.\Ejercicio5.ps1 -id 1,2 -nombre rick,morty


#>
# Función para imprimir la información de un personaje
param(
    [string[]]$id,
     [string[]]$nombre
)

function getInfo {
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
function getPersonajeId {

    $params = $id -join ','

    $filename = "personaje_$params.json"

    if (Test-Path $filename) {
        $response = Get-Content $filename -Raw
        getInfo $response
        return 
    }

    $url = "https://rickandmortyapi.com/api/character/$params"
    $response = Invoke-WebRequest $url


    if ($response.Count -eq 0) {
        Write-Host "Error: No se encontraron personajes con los IDs proporcionados."
        exit 1
    }
 
    $response | Set-Content $filename
    getInfo ($response )
}

# Función para realizar la búsqueda de personajes por nombre
function getPersonajeName {
    $Names = $nombre.Split(' ')

    foreach ($name in $Names) {
        $filename = "personaje_$name.json"

        if (Test-Path $filename) {
            $response = Get-Content $filename -Raw
            getInfo $response
            return 
        }
        
 

        $url = "https://rickandmortyapi.com/api/character/?name=$name"
        $response = Invoke-WebRequest $url | ConvertFrom-Json
        
        if ($response.Count -eq 0) {
            Write-Host "Error: No se encontraron resultados para el nombre '$name'."
        }
        else {
            $response.results | ConvertTo-Json | Set-Content $filename
            getInfo ($response.results | ConvertTo-Json)
        }
    }
}



# Verificar si se proporcionaron IDs y procesarlos

if ([string]::IsNullOrEmpty($id) -eq $false -and [string]::IsNullOrEmpty($nombre) -eq $false){
      getPersonajeId $id
      getPersonajeName $nombre
}else{

    if ([string]::IsNullOrEmpty($id) -eq $false) {
  
        getPersonajeId $id
    }
 
    # Verificar si se proporcionaron nombres y procesarlos
    if ([string]::IsNullOrEmpty($nombre) -eq $false) {
        getPersonajeName $nombre
    }
}

