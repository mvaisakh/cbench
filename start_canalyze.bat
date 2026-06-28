@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Project Cerium

echo Starting CAnalyze Web Server...
echo Running on http://localhost:8080
echo Press Ctrl+C to stop.

REM Change to the canalyze directory
cd /d "%~dp0canalyze"

REM Start the python HTTP server
python -m http.server 8080
