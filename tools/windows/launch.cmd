@echo off
rem Brave Fencer Musashi native port. Plays from your own disc image (.cue/.bin of
rem the USA disc, SLUS-00726); nothing from the game is in this package.
rem The disc is found in this order: %BFM_DISC% (a .cue or .bin), the one you picked
rem last time, disc\ next to this file, %USERPROFILE%\Games\brave-fencer-musashi\disc;
rem otherwise a file picker asks once and the choice is remembered.
setlocal DisableDelayedExpansion
for %%P in (%0) do set "HERE=%%~dpP"
set "CONF=%APPDATA%\bfm-port"
if not exist "%CONF%" mkdir "%CONF%"
set "LOG=%CONF%\last-run.log"
rem Launcher-managed data: explicit env paths never fall through to a picker.
set "LAUNCHER_DISC="
if defined BFM_DISC set "LAUNCHER_DISC=1"
if defined BFM_DISC_DIR set "LAUNCHER_DISC=1"
set "DISC=%BFM_DISC%"
if defined DISC if not exist "%DISC%" set "DISC="
if not defined DISC if defined BFM_DISC_DIR if exist "%BFM_DISC_DIR%\disc.cue" set "DISC=%BFM_DISC_DIR%\disc.cue"
if defined LAUNCHER_DISC if not defined DISC goto :missing_launcher_disc
if not defined DISC if exist "%CONF%\disc.txt" set /p DISC=<"%CONF%\disc.txt"
if defined DISC if not exist "%DISC%" set "DISC="
set "SEARCH_DIR=%HERE%disc"
if not defined DISC call :find
set "SEARCH_DIR=%USERPROFILE%\Games\brave-fencer-musashi\disc"
if not defined DISC call :find
if not defined DISC for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.OpenFileDialog; $d.Title='Choose your Brave Fencer Musashi (USA) .cue or .bin'; $d.Filter='Disc image|*.cue;*.bin'; if($d.ShowDialog() -eq 'OK'){$d.FileName}"`) do set "DISC=%%F"
if not defined DISC (
    echo bfm: No disc image found. Put your Brave Fencer Musashi ^(USA^) .cue and .bin in disc\ next to launch.cmd and start again.
    pause
    exit /b 66
)
> "%CONF%\disc.txt" <nul set /p "=%DISC%"
>> "%CONF%\disc.txt" echo.
rem Playable window, real-time guest clock, errors shown on screen.
if not defined MUSASHI_INTERACTIVE set MUSASHI_INTERACTIVE=1
if not defined MUSASHI_GUEST_CLOCK set MUSASHI_GUEST_CLOCK=1
if not defined MUSASHI_HOLD_WINDOW_MS set MUSASHI_HOLD_WINDOW_MS=3000
if not defined BFM_DEV_MENU (
    if exist "%CONF%\dev-menu-off" (set BFM_DEV_MENU=0) else (set BFM_DEV_MENU=1)
)
if not defined BFM_ROUTE_JOURNAL set "BFM_ROUTE_JOURNAL=%CONF%\route.jsonl"
echo bfm: running musashi_native_boot.exe> "%LOG%"
"%HERE%musashi_native_boot.exe" --disc "%DISC%" %* >> "%LOG%" 2>&1
set STATUS=%ERRORLEVEL%
if "%STATUS%"=="2" findstr /c:"could not start from your disc image" "%LOG%" >nul && del "%CONF%\disc.txt"
exit /b %STATUS%

:missing_launcher_disc
> "%LOG%" echo bfm: Launcher disc inputs are missing; check the requested disc path and try again.
exit /b 66

:find
if not exist "%SEARCH_DIR%\" exit /b 0
pushd "%SEARCH_DIR%" >nul 2>nul || exit /b 0
for %%F in (*.cue) do if not defined DISC if exist "%%~nxF" set "DISC=%SEARCH_DIR%\%%~nxF"
for %%F in (*.bin) do if not defined DISC if exist "%%~nxF" set "DISC=%SEARCH_DIR%\%%~nxF"
popd
exit /b 0
