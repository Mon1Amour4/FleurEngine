@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "SCRIPT_DIR=%~dp0"
set "GLSLANG=C:\VulkanSDK\1.4.335.0\Bin\glslangValidator.exe"
set "SPIRV_VAL=C:\VulkanSDK\1.4.335.0\Bin\spirv-val.exe"
set "RESULT=1"
set "FAIL_REASON=unknown error"
set "IN_SHADER_DIR=0"

if not exist "%GLSLANG%" (
    set "FAIL_REASON=glslangValidator.exe not found: %GLSLANG%"
    goto :report
)
if not exist "%SPIRV_VAL%" (
    set "FAIL_REASON=spirv-val.exe not found: %SPIRV_VAL%"
    goto :report
)
pushd "%SCRIPT_DIR%"
if errorlevel 1 (
    set "FAIL_REASON=failed to enter shader directory: %SCRIPT_DIR%"
    goto :report
)
set "IN_SHADER_DIR=1"

for %%F in (*.vert) do (
    set "OUTPUT=%%~nF.spv"
    if /I "%%~nF"=="pointLightShadow" set "OUTPUT=pointLightShadowVertex.spv"
    echo [COMPILE][VERT] %%~nxF
    "%GLSLANG%" -V -gVS -Od -S vert -o "!OUTPUT!" "%%~fF"
    if errorlevel 1 (
        set "FAIL_REASON=failed to compile %%~nxF"
        goto :report
    )
)

for %%F in (*.frag) do (
    set "OUTPUT=%%~nF.spv"
    if /I "%%~nF"=="pointLightShadow" set "OUTPUT=pointLightShadowFragment.spv"
    echo [COMPILE][FRAG] %%~nxF
    "%GLSLANG%" -V -gVS -Od -S frag -o "!OUTPUT!" "%%~fF"
    if errorlevel 1 (
        set "FAIL_REASON=failed to compile %%~nxF"
        goto :report
    )
)

for %%F in (*.geom) do (
    set "OUTPUT=%%~nF.spv"
    if /I "%%~nF"=="pointLightShadow" set "OUTPUT=pointLightShadowGeometry.spv"
    echo [COMPILE][GEOM] %%~nxF
    "%GLSLANG%" -V -gVS -Od -S geom -o "!OUTPUT!" "%%~fF"
    if errorlevel 1 (
        set "FAIL_REASON=failed to compile %%~nxF"
        goto :report
    )
)

for %%F in (*.spv) do (
    echo [VALIDATE] %%~nxF
    if not exist "%%~fF" (
        set "FAIL_REASON=missing output %%~nxF"
        goto :report
    )
    "%SPIRV_VAL%" "%%~fF"
    if errorlevel 1 (
        set "FAIL_REASON=SPIR-V validation failed for %%~nxF"
        goto :report
    )
)

set "RESULT=0"
:report
echo.
if "%RESULT%"=="0" (
    echo [SUCCESS] All shaders compiled and validated successfully.
) else (
    echo [ERROR] %FAIL_REASON%
)

if "%IN_SHADER_DIR%"=="1" popd
pause
exit /b %RESULT%
