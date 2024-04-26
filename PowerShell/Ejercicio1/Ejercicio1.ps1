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
#   Instancia de entrega: Primera Entrega               #
#                                                       #
#########################################################


<#
.SYNOPSIS
Script para resuma la información de todos los archivos CSV en un único archivo JSON

.DESCRIPTION
Una vez finalizadas todas las mesas de examen, se corre un proceso que se encarga de realizar un resumen de las notas de cada alumno para luego poder publicarlo en un sitio web. 
Para calcular la nota de cada final, se debe tener en cuenta que: 
• Cada ejercicio tiene el mismo peso en la nota final. Para calcularlo se puede usar la siguiente fórmula: 10 / CantidadEjercicios. 
• Un ejercicio bien (B) vale el ejercicio entero. • Un ejercicio regular (R) vale medio ejercicio. 
• Un ejercicio mal (M) no suma puntos a la nota final. 
• Cada materia puede tener diferente cantidad de ejercicios. 

.PARAMETER directorio
Ruta del directorio que contiene los archivos CSV a procesar. 

.PARAMETER salida
Ruta del archivo JSON de salida. 

.PARAMETER pantalla
Muestra la salida por pantalla, no genera el archivo JSON. Este parámetro no se puede usar a la vez que -s. 

.EXAMPLE
.\Ejercicio1.ps1 -directorio "C:\ruta\del\directorio" -salida "C:\ruta\del\archivo.json" 

.NOTES
Consideraciones: 
• Se asume que solamente hay un archivo de notas por materia. 
• La nota calculada se trunca para no tener decimales. 
#>


param(
    [Parameter(Mandatory=$true)] [ValidateNotNullOrEmpty()]
    [string]$directorio,

    [Parameter(Mandatory=$false)] [ValidateNotNullOrEmpty()]
    [string]$salida,

    [Parameter(Mandatory=$false)]
    [switch]$pantalla
)


# Función para calcular la nota final de un alumno en una materia
function CalcularNota($notas) {
    $peso = 10 / ($notas.Count - 1) # Restamos 1 para excluir el DNI del cálculo
    $nota_final = 0

    foreach ($nota in $notas) {
        switch ($nota) {
            "B" { $nota_final += $peso * 1 }
            "R" { $nota_final += $peso * 0.5 }
            "M" { $nota_final += 0 }
        }
    }

    return [math]::Truncate($nota_final)
}

# Lista para almacenar el resumen de notas
$notas_alumnos = @()

# Recorre los archivos CSV en el directorio especificado
foreach ($archivo in Get-ChildItem -Path $directorio -Filter *.csv) {
    $materia = [System.IO.Path]::GetFileNameWithoutExtension($archivo.Name)
    $contenido = Get-Content $archivo.FullName | Select-Object -Skip 1  # Omitir la primera línea

    foreach ($linea in $contenido) {
        $datos = $linea.Split(',')
        $dni = $datos[0]
        $notas = $datos[1..($datos.Count - 1)]
       # Write-Output $datos

        # Calcula la nota final
        $nota_final = CalcularNota $notas
        #Write-Output $nota_final

        # Verifica si el alumno ya está en la lista
        $alumno_existente = $notas_alumnos | Where-Object { $_.dni -eq $dni }

        # Si el alumno no está en la lista, lo agrega
        if (-not $alumno_existente) {
            $alumno = @{
                "dni" = $dni
                "notas" = @()
            }
            $notas_alumnos += New-Object PSObject -Property $alumno
            $alumno_existente = $notas_alumnos | Where-Object { $_.dni -eq $dni }
        }

        # Agrega la nota de la materia al alumno
        $materia_nota = @{
            "materia" = [int]$materia
            "nota" = $nota_final
        }
        $alumno_existente.notas += New-Object PSObject -Property $materia_nota
    }
}

# Genera el archivo JSON
$json_output = @{
    "notas" = $notas_alumnos
} | ConvertTo-Json

# Muestra el resultado por pantalla si se especificó el parámetro -pantalla
if ($pantalla) {
    #$notasFinales | ConvertTo-Json | Write-Output
    Write-Output $json_output
} else {
    # Guarda el resultado en el archivo especificado
    $json_output | Out-File -FilePath $salida
    Write-Output "El archivo fue generado exitosamente en la ruta indicada"
}