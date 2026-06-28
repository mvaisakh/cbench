# 📊 CAnalyze: Visual Benchmark Comparison

`cbench` includes **CAnalyze**, a sleek, cross-platform Single Page Web Application (SPA) designed with Material Design 3. It allows kernel developers to easily compare the JSON telemetry outputs of two benchmark runs (e.g., "Before" and "After" a kernel patch) to instantly visualize performance improvements and regressions.

## Features
- **100% Offline**: CAnalyze runs completely locally using bundled libraries (Chart.js).
- **Performance Radar**: Automatically normalizes and projects metrics onto an interactive radar chart.
- **Automated Deltas**: Highlights improvements in green and regressions in red, accounting for metric types (e.g., lower latency vs higher throughput).

## Using CAnalyze

1. Run your benchmarks before and after your kernel changes, saving the JSON output:
   ```bash
   sudo ./cbench -a -d 15 -j > before.json
   # Apply kernel patch, reboot...
   sudo ./cbench -a -d 15 -j > after.json
   ```

2. Start the local HTTP server from the root of the repository using the provided scripts:
   - On **Linux/macOS**: Run `./start_canalyze.sh`
   - On **Windows**: Double-click `start_canalyze.bat`

3. Open your web browser and navigate to `http://localhost:8080`.

4. Drag and drop `before.json` and `after.json` into the respective drop zones and click **Analyze Benchmarks**.

CAnalyze will automatically align metrics, calculate percentage deltas, color-code regressions (red) and improvements (green), and display any heuristics or patching advice side-by-side.
