@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "GLSLANG=C:\VulkanSDK\1.4.335.0\Bin\glslangValidator.exe"
set "SPIRV_VAL=C:\VulkanSDK\1.4.335.0\Bin\spirv-val.exe"

if not exist "%GLSLANG%" exit /b 1
if not exist "%SPIRV_VAL%" exit /b 1
pushd "%SCRIPT_DIR%" || exit /b 1

"%GLSLANG%" -V -gVS -Od -S vert -o opaqueVertex.spv opaqueVertex.vert || exit /b 1
"%GLSLANG%" -V -gVS -Od -S frag -o opaqueFragment.spv opaqueFragment.frag || exit /b 1
"%GLSLANG%" -V -gVS -Od -S vert -o shadowVertex.spv shadowVertex.vert || exit /b 1
"%GLSLANG%" -V -gVS -Od -S geom -o shadowGeometry.spv shadowGeometry.geom || exit /b 1
"%GLSLANG%" -V -gVS -Od -S frag -o shadowFragment.spv shadowFragment.frag || exit /b 1

for %%F in (opaqueVertex.spv opaqueFragment.spv shadowVertex.spv shadowGeometry.spv shadowFragment.spv) do (
    if not exist "%%F" exit /b 1
    "%SPIRV_VAL%" "%%F" || exit /b 1
)

popd
exit /b 0
