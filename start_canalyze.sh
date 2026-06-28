#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Project Cerium

echo "Starting CAnalyze Web Server..."
echo "Running on http://localhost:8080"
echo "Press Ctrl+C to stop."

# Change to the canalyze directory
cd "$(dirname "$0")/canalyze"

# Start the python HTTP server
python3 -m http.server 8080
