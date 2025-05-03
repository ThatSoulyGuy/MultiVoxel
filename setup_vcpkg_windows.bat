@echo on
setlocal

:: 0) Where we live
set "SCRIPT_DIR=%~dp0"
set "VCPKG_ROOT=%SCRIPT_DIR%vcpkg"

:: 1) Which triplet?  Default to x64-windows-static
if "%~1"=="" (
  set "TRIPLET=x64-windows-static"
) else (
  set "TRIPLET=%~1"
)
echo [*] Using vcpkg triplet: %TRIPLET%

:: 2) Clone & bootstrap vcpkg if needed
if not exist "%VCPKG_ROOT%\bootstrap-vcpkg.bat" (
  echo [*] Cloning vcpkg…
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
)
pushd "%VCPKG_ROOT%"
call bootstrap-vcpkg.bat
popd

:: 3) Install ONLY OpenSSL & Protobuf
echo [*] Installing OpenSSL and Protobuf…
"%VCPKG_ROOT%\vcpkg.exe" install openssl:%TRIPLET%
"%VCPKG_ROOT%\vcpkg.exe" install protobuf:%TRIPLET%

"%VCPKG_ROOT%\vcpkg.exe" integrate install

:: 4) Generate toolchain.generated.cmake
:: 4a) Create a forward-slash version of VCPKG_ROOT for CMake paths
set "VCPKG_ROOT_FS=%VCPKG_ROOT:\=/%"
echo [*] VCPKG_ROOT for CMake: %VCPKG_ROOT_FS%

:: 4b) Generate toolchain.generated.cmake with forward-slashes
set "TOOLCHAIN=%SCRIPT_DIR%toolchain.generated.cmake"
>"%TOOLCHAIN%" echo # Auto-generated toolchain for MultiVoxel
>>"%TOOLCHAIN%" echo.
>>"%TOOLCHAIN%" echo # 1) Use vcpkg’s own CMake integration
>>"%TOOLCHAIN%" echo set(CMAKE_TOOLCHAIN_FILE "%VCPKG_ROOT_FS%/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "" FORCE)
>>"%TOOLCHAIN%" echo set(VCPKG_TARGET_TRIPLET "%TRIPLET%"                    CACHE STRING  "" FORCE)
>>"%TOOLCHAIN%" echo.
>>"%TOOLCHAIN%" echo # 2) Make vcpkg’s installed folder discoverable to find_package()
>>"%TOOLCHAIN%" echo list(APPEND CMAKE_PREFIX_PATH "%VCPKG_ROOT_FS%/installed/%TRIPLET%")
>>"%TOOLCHAIN%" echo.
>>"%TOOLCHAIN%" echo # 3) Point CMake at OpenSSL from vcpkg
>>"%TOOLCHAIN%" echo set(OpenSSL_ROOT_DIR "%VCPKG_ROOT_FS%/installed/%TRIPLET%" CACHE PATH "" FORCE)
>>"%TOOLCHAIN%" echo.
>>"%TOOLCHAIN%" echo # 4) Point CMake at Protobuf from vcpkg
>>"%TOOLCHAIN%" echo set(Protobuf_DIR "%VCPKG_ROOT_FS%/installed/%TRIPLET%/share/protobuf" CACHE PATH "" FORCE)

echo [*] Wrote %TOOLCHAIN%

echo [✓] Done!
endlocal