@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo ========================================
echo   Claw++ v1.0.0 Installer Builder
echo ========================================
echo.

set QT_DIR=C:\Qt\6.5.3\mingw_64
set IFW_DIR=C:\Qt\Tools\QtInstallerFramework\4.11
set BUILD_DIR=build
set RELEASE_DIR=build\install
set PACKAGE_DIR=config\package

echo [1/7] Checking Qt and IFW paths...
if not exist "%QT_DIR%" (
    echo ERROR: Qt directory not found at %QT_DIR%
    pause
    exit /b 1
)
if not exist "%IFW_DIR%\bin\binarycreator.exe" (
    echo ERROR: Qt Installer Framework not found at %IFW_DIR%
    pause
    exit /b 1
)
echo   Qt: %QT_DIR%
echo   IFW: %IFW_DIR%
echo.

echo [2/7] Setting up MinGW environment...
set PATH=%QT_DIR%\bin;%PATH%
where mingw32-make >nul 2>&1
if errorlevel 1 (
    echo ERROR: mingw32-make not found in PATH
    echo Please add %QT_DIR%\bin to your PATH
    pause
    exit /b 1
)
echo   MinGW ready.
echo.

echo [3/7] Cleaning previous build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo   Cleaned.
echo.

echo [4/7] Configuring with CMake (MinGW)...
cmake -G "MinGW Makefiles" -B "%BUILD_DIR%" -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%RELEASE_DIR%" -DCMAKE_PREFIX_PATH="%QT_DIR%"
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)
echo.

echo [5/7] Building Release...
cmake --build "%BUILD_DIR%" -- -j8
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)
echo   Build complete.
echo.

echo [6/7] Installing to staging directory...
cmake --install "%BUILD_DIR%"
if errorlevel 1 (
    echo ERROR: Install failed
    pause
    exit /b 1
)
echo   Staged to %RELEASE_DIR%

echo   Copying clawrunner_internal...
if exist "%BUILD_DIR%\bin\clawrunner_internal.exe" (
    copy /y "%BUILD_DIR%\bin\clawrunner_internal.exe" "%RELEASE_DIR%\bin\clawrunner_internal.exe" >nul
    echo   Copied.
) else (
    echo   Searching for clawrunner_internal.exe...
    for /r "%BUILD_DIR%" %%F in (clawrunner_internal.exe) do (
        copy /y "%%F" "%RELEASE_DIR%\bin\clawrunner_internal.exe" >nul
        echo   Found and copied: %%F
    )
)
echo.

echo [7/7] Creating installer package...
if exist "%PACKAGE_DIR%\packages\org.clawpp.root\data" rmdir /s /q "%PACKAGE_DIR%\packages\org.clawpp.root\data"
mkdir "%PACKAGE_DIR%\packages\org.clawpp.root\data"

xcopy "%RELEASE_DIR%\*" "%PACKAGE_DIR%\packages\org.clawpp.root\data\" /E /I /H /Y >nul
echo   Package data prepared.

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
