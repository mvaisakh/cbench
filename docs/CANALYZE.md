# 📊 CAnalyze: Visual Benchmark Comparison & Analytics

`cbench` includes **CAnalyze**, a sleek, cross-platform Single Page Web Application (SPA) designed with Material Design 3. It allows kernel developers to easily analyze standalone benchmark telemetry or compare two runs (e.g., "Before" and "After" a kernel patch) to instantly isolate performance gains, regressions, and hardware bottlenecks.

## Features
- **100% Offline & Zero CDNs**: CAnalyze runs completely locally using bundled libraries (`chart.min.js`).
- **Single-Run & Comparison Modes**: Inspect standalone benchmark metrics or compare two runs side-by-side.
- **Hardware Telemetry & Vital Signs**: Automatically extracts and highlights peak thermal metrics, hardware IPC, L1D cache miss rates, branch mispredictions, CPU frequencies, and energy efficiency.
- **Interactive Visualizations**: Toggle between Performance Radar view and Grouped Relative Bar charts with subsystem aggregation or fine-grained detailed filtering.
- **Search, Filter & Sort**: Instantly search metrics, filter by subsystem chips, and sort columns by value or delta.
- **Automated Deltas & Severity Heuristics**: Color-coded improvements (green) and regressions (red), paired with categorized kernel patch advice (Critical, Warning, Optimization).
- **1-Click Reporting & Export**:
  - **Export Markdown Report**: Generates a clean, copy-paste-ready report formatted for GitHub pull requests and kernel mailing list patches.
  - **Export CSV**: Export all aligned metrics for spreadsheet analysis.
  - **Export JSON**: Download raw benchmark run telemetry.

## Using CAnalyze

1. Run your benchmarks before and after your kernel changes, saving the JSON output:
   ```bash
   sudo ./cbench -a -d 15 -o before.json
   # Apply kernel patch, reboot...
   sudo ./cbench -a -d 15 -o after.json
   ```

2. Start the local HTTP server from the root of the repository using the provided scripts:
   - On **Linux/macOS**: Run `./start_canalyze.sh`
   - On **Windows**: Double-click `start_canalyze.bat`

3. Open your web browser and navigate to `http://localhost:8080`.

4. Drag and drop your JSON files into the drop zones or import them from the top navigation bar.

CAnalyze will automatically align metrics, calculate percentage deltas, categorize bottlenecks, and display actionable kernel tuning recommendations.

