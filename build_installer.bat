@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo ========================================
echo   Claw++ v1.0.0 Installer Builder
echo ========================================
echo.

set QT_DIR=C:\Qt\6.5.3\msvc2019_64
set IFW_DIR=C:\Qt\Tools\QtInstallerFramework\4.7
set BUILD_DIR=build
set RELEASE_DIR=build\install
set PACKAGE_DIR=config\package

echo [1/6] Checking Qt and IFW paths...
if not exist "%QT_DIR%" (
    echo ERROR: Qt directory not found at %QT_DIR%
    echo Please update QT_DIR in build_installer.bat
    pause
    exit /b 1
)
if not exist "%IFW_DIR%\bin\binarycreator.exe" (
    echo ERROR: Qt Installer Framework not found at %IFW_DIR%
    echo Please update IFW_DIR in build_installer.bat or install Qt IFW
    pause
    exit /b 1
)
echo   Qt: %QT_DIR%
echo   IFW: %IFW_DIR%
echo.

echo [2/6] Cleaning previous build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo   Cleaned.
echo.

echo [3/6] Building Release...
cmake -B "%BUILD_DIR%" -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%RELEASE_DIR%"
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)
cmake --build "%BUILD_DIR%" --config Release -j8
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)
echo   Build complete.
echo.

echo [4/6] Installing to staging directory...
cmake --install "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo ERROR: Install failed
    pause
    exit /b 1
)
echo   Staged to %RELEASE_DIR%
echo.

echo [5/6] Copying clawrunner_internal to install dir...
if exist "%BUILD_DIR%\bin\clawrunner_internal.exe" (
    copy /y "%BUILD_DIR%\bin\clawrunner_internal.exe" "%RELEASE_DIR%\bin\clawrunner_internal.exe" >nul
    echo   Copied clawrunner_internal.exe
) else (
    echo WARNING: clawrunner_internal.exe not found, searching...
    for /r "%BUILD_DIR%" %%F in (clawrunner_internal.exe) do (
        copy /y "%%F" "%RELEASE_DIR%\bin\clawrunner_internal.exe" >nul
        echo   Found and copied: %%F
    )
)
echo.

echo [6/6] Creating installer package...
rem Clean old package data
if exist "%PACKAGE_DIR%\packages\org.clawpp.root\data" rmdir /s /q "%PACKAGE_DIR%\packages\org.clawpp.root\data"
mkdir "%PACKAGE_DIR%\packages\org.clawpp.root\data"

rem Copy all installed files to package data
xcopy "%RELEASE_DIR%\*" "%PACKAGE_DIR%\packages\org.clawpp.root\data\" /E /I /H /Y >nul
echo   Package data prepared.

rem Generate installer
"%IFW_DIR%\bin\binarycreator.exe" -c "%PACKAGE_DIR%\config.xml" -p "%PACKAGE_DIR%\packages" ClawPP-Installer-v1.0.0.exe
if errorlevel 1 (
    echo ERROR: binarycreator failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Installer built: ClawPP-Installer-v1.0.0.exe
echo ========================================
echo.
pause
