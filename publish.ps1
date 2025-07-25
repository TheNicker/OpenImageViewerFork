# PowerShell Build Script

#===================================================================
# For using this build and packaging script you'll need to define
# 1. 7z Path.
# 2. git Path.
# 3. [optional] set OIV_OFFICIAL_BUILD to 1 if it's an official build.
#====================================================================

# Build script operations
$OpRunCmake = 1
$OpBuild = 1
$OpPack = 1

# Set custom paths
$SevenZipPath = "C:/Program Files/7-Zip"
$GitPath = "C:/Program Files/Git/bin"
$DependenciesPath = "./oiv/Dependencies"

# Change to 1 to make an official build
$OIV_OFFICIAL_BUILD=1
$OIV_OFFICIAL_RELEASE=1
$OIV_RELEASE_SUFFIX = ""
$OIV_VERSION_BUILD=12
$BuildType = "RelWithDebInfo"
$VersionPath = "./oivlib/oiv/Include/Version.h"
$BuildPath = "./publish"
$BinPath = "$BuildPath/bin"


function RaiseError
{
    param
    (
        [string]$message
    )

    $lastColor =  [Console]::ForegroundColor;
    [Console]::ForegroundColor = [ConsoleColor]::Red
    
    write-host $message
    # Reset to previous color 
    [Console]::ForegroundColor = $lastColor;
    [Console]::ResetColor()
    
    exit  1
}

function GetNumber
{
param
    (
        [string]$versionString
    )

        $match = $versionString | Select-String -Pattern "\d+"

        if ($match) 
        {
        #    Write-Output "Match found: $($match.Matches[0].Value)"
            return [int]$match.Matches[0].ToString()
        } 
        else
        {
            RaiseError "Cannot parse version number"
        }
}


function GetVersionNumber
{
    param
    (
        [string]$content
        ,[string]$token
    )
        $match = $content | Select-String -Pattern "${token}.*=.*\d"

        if ($match) 
        {
         #   Write-Output "Match found: $($match.Matches[0].Value)"
            return GetNumber  $match.Matches[0].ToString()
        } 
        else
        {
            RaiseError "Cannot parse version"
        }
}


# Run vswhere to find the latest installed version of Visual Studio
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInstallations = & $vswherePath -latest -requires Microsoft.Component.MSBuild -property installationPath

# Check if vswhere found any Visual Studio installation
if ($vsInstallations) 
{
    $vsdir = $vsInstallations
    $vsdir = $vsdir -replace '\\', '/'

    $CMakePath = & $vswherePath -products * -latest -find **\cmake.exe
    $CMakePath = $CMakePath -replace '\\', '/'

    $NinjaPath = & $vswherePath -products * -latest -find **\ninja.exe
    $NinjaPath = $NinjaPath -replace '\\', '/'

    #$Vars64 = & $vswherePath -products * -latest -find **\vcvars64.bat
} 
else 
{
    Write-Host "Visual Studio or MSBuild not found."
}


# Update path
$env:Path = "$env:Path;$MSBuildPath;$SevenZipPath;$GitPath;$CMakePath;$NinjaPath"

$DATE_YYMMDD = Get-Date -Format "yyyy-MM-dd"
$DATE_YYMMDD_HH_mm_SS = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"

$fileContent = Get-Content -Path $PSScriptRoot/$VersionPath

$major=GetVersionNumber $fileContent  "OIV_VERSION_MAJOR"
$minor=GetVersionNumber $fileContent "OIV_VERSION_MINOR"
$revision=$(git rev-list HEAD --count)


$versionString = "${major}.${minor}.$revision.$OIV_RELEASE_SUFFIX$OIV_VERSION_BUILD"
$versionStringShort = $versionString

if ($OIV_OFFICIAL_RELEASE -eq 0) 
{
    $OIV_VERSION_REVISION = git rev-parse --short HEAD
    $versionString = "$versionStringShort-$OIV_VERSION_REVISION-Nightly"
}

Write-Host "==============================================="
Write-Host "FOUND VERSION: $versionString"
Write-Host "SHORT VERSION: $versionStringShort"
Write-Host "SHORT DATE   : $DATE_YYMMDD"
Write-Host "LONG DATE    : $DATE_YYMMDD_HH_mm_SS"
Write-Host "==============================================="



# Run Cmake
if ($OpRunCmake -eq 1) 
{
    & $CMakePath "-S ." "-B $BuildPath" "-G Ninja" `
        "-DCMAKE_BUILD_TYPE=$BuildType" `
        "-DCMAKE_MT=$VSdir/VC/Tools/Llvm/x64/bin/llvm-mt.exe" `
        "-DCMAKE_C_COMPILER=$VSdir/VC/Tools/Llvm/x64/bin/clang-cl.exe" `
        "-DCMAKE_CXX_COMPILER=$VSdir/VC/Tools/Llvm/x64/bin/clang-cl.exe" `
        "-DCMAKE_MAKE_PROGRAM=$NinjaPath" `
        "-DCMAKE_RC_COMPILER=$VSdir/VC/Tools/Llvm/x64/bin/llvm-rc.exe" `
        "-DIMCODEC_BUILD_CODEC_FREEIMAGE=ON" `
        "-DOIV_OFFICIAL_BUILD=${OIV_OFFICIAL_BUILD}" `
        "-DOIV_OFFICIAL_RELEASE=$OIV_OFFICIAL_RELEASE" `
        "-DOIV_VERSION_BUILD=$OIV_VERSION_BUILD" `
        "-DOIV_RELEASE_SUFFIX=L`"$OIV_RELEASE_SUFFIX`""

        

    if (-not $?) {RaiseError "CMake configuration failed. Please check if CMake is installed properly."}
}


# Build project
if ($OpBuild -eq 1) 
{
   # & $Vars64
    
   # if (-not $?)  { RaiseError "Cannot set Visual studio environment"}

    Set-Location -Path $BuildPath
    & $NinjaPath

    $buildResult = $?
    Set-Location -Path -

    if (-not $buildResult)  { RaiseError "Compilation error"}
    
}

# Pack files
if ($OpPack -eq 1) 
{
    $OutputPath = "./$BuildPath/$DATE_YYMMDD_HH_mm_SS-v$versionStringShort"
    Copy-Item "$DependenciesPath\*.dll" -Destination $BinPath -Force
    New-Item -ItemType Directory -Force -Path $OutputPath
    $BaseFileName = "$OutputPath\$DATE_YYMMDD-OIV-$versionString-Win32x64VC-LLVM"

    # Pack symbols into 7z file
    & "$SevenZipPath\7z" a -mx9 "$BaseFileName-Symbols.7z" "$BinPath\*.pdb"

    # Pack application into 7z file
    & "$SevenZipPath\7z" a -mx9 "$BaseFileName.7z" "$BinPath\*.dll" "$BinPath\*.exe" "$BinPath\Resources"

    if (-not $?) { RaiseError "Cannot pack files"}
}

Write-Host "=========="
Write-Host "Success!"
Write-Host "=========="
