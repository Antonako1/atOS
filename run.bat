@echo off

:: Install Winget
winget --version
if %errorlevel% neq 0 (
    echo Installing Winget...
    powershell.exe -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser; iwr -useb https://aka.ms/installwinget | iex"
)

:: Install NPM and Node.js
echo Installing NPM and Node.js...
winget install -e --id Node.NodeJS

node main.js

pause