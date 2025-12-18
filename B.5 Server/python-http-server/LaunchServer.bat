@echo off
title Smart Mailbox Dashboard
color 0A

echo ========================================================
echo          SMART MAILBOX PROJECT LAUNCHER
echo ========================================================
echo.

:: --- STEP 1: INSTALL/UPDATE REQUIREMENTS ---
echo [1/4] Checking Python libraries...
pip install flask python-dotenv
if %errorlevel% neq 0 (
    color 0C
    echo.
    echo [ERROR] Python or PIP is not installed or not in your PATH.
    echo Please install Python from python.org and try again.
    pause
    exit /b
)
echo Libraries are ready.
echo.

:: --- STEP 2: CHECK CONFIGURATION ---
echo [2/4] Checking configuration file...
if not exist .env (
    color 0E
    echo.
    echo [WARNING] .env file was not found!
    echo I have created a template .env file for you.
    echo Please open ".env", add your passwords, and run this script again.
    
    :: Create a default .env file
    echo SERVER_PORT=3000> .env
    echo AUTH_TOKEN=BC5FB17A739C64639751B59209E07F88>> .env
    echo EMAIL_SENDER=my-iot-project@gmail.com>> .env
    echo EMAIL_PASSWORD=REPLACE_WITH_APP_PASSWORD>> .env
    echo EMAIL_RECIPIENT=your_personal_email@gmail.com>> .env
    
    pause
    exit /b
)
echo Configuration found.
echo.

:: --- STEP 3: START NGROK (NEW WINDOW) ---
echo [3/4] Launching Ngrok Tunnel...
:: This opens a separate popup window for Ngrok so it doesn't block the script
start "Ngrok Tunnel" ngrok http --domain=unarithmetically-peppiest-libbie.ngrok-free.dev 3000

:: --- STEP 4: START PYTHON SERVER ---
echo [4/4] Starting Python Server...
echo.
echo ========================================================
echo    Dashboard: https://unarithmetically-peppiest-libbie.ngrok-free.dev
echo    Local:     http://localhost:3000
echo    Status:    RUNNING (Keep this window open)
echo ========================================================
echo.

python server.py

:: If python crashes, keep window open to see error
pause