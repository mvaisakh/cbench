# Cerium Benchmarking (`cbench`)

`cbench` is a highly specialized, statically compiled Linux kernel benchmarking and telemetry tool. Designed with zero external dependencies, it is explicitly built to stress-test core kernel subsystems, track hardware-level energy consumption, and automatically isolate algorithmic and architectural bottlenecks in the Linux Kernel.

## Features
- **Topology-Aware Threading**: Automatically detects CPU clusters (e.g., ARM `silver`/`gold` clusters) and binds threads appropriately for accurate scaling.
- **Hardware Energy Subsystem**: Integrates with Intel RAPL (`energy_uj`) and Android hardware sensors (Battery Management Systems, `current_now`, `energy_now`) to calculate operations-per-Joule efficiency.
- **Automated Bottleneck Analyzer**: Monitors CPU Idle Residency (`cpuidle` C-states vs. WFI) during benchmarks to definitively prove if a bottleneck is algorithmic (100% CPU usage) or due to severe hardware/lock contention (>90% idle time).
- **Native Kernel Profiler**: Uses `perf_event_open` and `/proc/kallsyms` to sample and pinpoint the exact C-functions dominating CPU time.
- **Symbolic Heuristics Engine**: Scans kernel profiler hotspots against known anti-patterns to provide direct patching advice (e.g., detecting `_raw_spin_lock` and suggesting RCU, or detecting `clear_page` and suggesting THP).
- **JSON Telemetry Export**: Dumps all metrics into a structured JSON file for CI/CD dashboard tracking.
- **CAnalyze GUI**: A built-in Material Design web application to visually compare benchmark JSON outputs, flag performance regressions, and present kernel patching heuristics.

## Benchmarks
1. **Syscall Latency (`SYS_gettid`)**: Measures raw context-switching overhead across the User/Kernel boundary.
2. **Scheduler Context Switching**: Measures inter-process communication and scheduling overhead via tightly coupled pipe ping-pongs.
3. **Memory Subsystem**: Aggregates throughput for sequential aligned writes and fast `memcpy` operations.
4. **I/O Subsystem**: Measures buffered write bandwidth (forcing `fsync` journal flushes) and cached read bandwidth.
5. **RNG Subsystem**: Threads constantly `read()` from `/dev/urandom` to stress the kernel entropy pool, ChaCha20 PRNG, and `drivers/char/random.c` locks.
6. **Network Stack**: UDP loopback (`127.0.0.1`) blasting to stress `sk_buff` allocation, network namespaces, and the TCP/IP stack (`net/core/dev.c`).
7. **Futex Contention**: Threads heavily contest a shared futex via `syscall(SYS_futex)`. This is the #1 bottleneck for the Android Java Runtime (ART).
8. **Kernel Crypto API (`AF_ALG`)**: Binds to `AF_ALG` sockets to compute `sha256` hashes, testing if the kernel is leveraging hardware cryptography (like Qualcomm CE).
9. **Zero / Copy-to-User**: Massive concurrent `read()`s from `/dev/zero` to isolate and stress `clear_page` and `copy_to_user` memory operations.

---

## Building `cbench`

`cbench` relies heavily on standard POSIX interfaces and Linux kernel telemetry nodes. It is built as a pure static binary.

### Standard Linux (x86_64 / ARM64)
Ensure you have the static C libraries installed on your host system (e.g., `glibc-static` on Fedora/RHEL).
```bash
make clean
make
```

### Android (Cross-Compilation via NDK)
Android does not use standard `glibc`. To properly cross-compile `cbench` for an Android device, you should use the prebuilt Clang compiler provided by the Android NDK (Native Development Kit):
```bash
make clean
CC=/path/to/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang make
```

---

## Usage

You must run `cbench` as `root` (`sudo` or `su`) so that it can access `/sys` topology nodes, Intel/Android power sensors, and the `perf_event_open` kernel profiler rings!

**Run all benchmarks for 15 seconds each, utilizing all CPU cores, and print JSON at the end:**
```bash
sudo ./cbench -a -d 15 -j
```

**Options:**
- `-a` : Run all subsystem benchmarks.
- `-d N` : Set the duration of each benchmark to `N` seconds (default: 10).
- `-t N` : Explicitly specify the number of threads to spawn (default: total CPU cores).
- `-s` : Run only the Syscall Latency benchmark.
- `-S` : Run only the Scheduler Context Switch benchmark.
- `-m` : Run only the Memory Bandwidth benchmark.
- `-i` : Run only the I/O Bandwidth benchmark.
- `-r` : Run only the RNG benchmark (`/dev/urandom`).
- `-n` : Run only the Network loopback benchmark.
- `-f` : Run only the Futex contention benchmark.
- `-c` : Run only the Kernel Crypto API (`AF_ALG`) benchmark.
- `-z` : Run only the Zero / Copy-to-User benchmark (`/dev/zero`).
- `-j` : Print structured JSON metrics at the end of the run.

---

## 📊 CAnalyze: Visual Benchmark Comparison

`cbench` includes **CAnalyze**, a sleek, cross-platform Single Page Web Application (SPA) designed with Material Design 3. It allows kernel developers to easily compare the JSON telemetry outputs of two benchmark runs (e.g., "Before" and "After" a kernel patch) to instantly visualize performance improvements and regressions.

### Using CAnalyze

1. Run your benchmarks before and after your kernel changes, saving the JSON output:
   ```bash
   sudo ./cbench -a -d 15 -j > before.json
   # Apply kernel patch, reboot...
   sudo ./cbench -a -d 15 -j > after.json
   ```
2. Navigate to the `canalyze` directory and start the local HTTP server using the provided scripts:
   - On **Linux/macOS**: Run `./start.sh`
   - On **Windows**: Double-click `start.bat`
3. Open your web browser and navigate to `http://localhost:8080`.
4. Drag and drop `before.json` and `after.json` into the respective drop zones and click **Analyze Benchmarks**.

CAnalyze will automatically align metrics, calculate percentage deltas, color-code regressions (red) and improvements (green), and display any heuristics or patching advice side-by-side.

---

## 📱 Android-Specific Profiling & Execution Notes

Android enforces an extremely strict SELinux and mount security model. To execute `cbench` and get accurate kernel profiling on an Android device, you must follow these exact steps:

### 1. Push to the Developer Temp Directory
Android mounts standard user partitions (like `/sdcard/`) with a `noexec` flag, which prevents binaries from running. You **must** push `cbench` directly to the shell developer directory.

From your host computer:
```bash
adb push cbench /data/local/tmp/
```

### 2. Disable Kernel Pointer Restrictions (`kptr_restrict`)
By default, Android's security policy sets `/proc/sys/kernel/kptr_restrict` to `2`. This forcibly hides all real memory addresses in `/proc/kallsyms`, causing the `cbench` profiler to throw `[unmapped_proprietary_code]` errors because it cannot map memory addresses to kernel symbols.

From your host computer, drop into the shell, escalate to root, fix the security constraints, and run the benchmark:

```bash
adb shell
su
cd /data/local/tmp/
chmod +x cbench
```

# REQUIRED:
## Telemetry & OS Insights
Every benchmark is wrapped in a highly precise telemetry envelope that passively tracks kernel performance without altering the benchmark payload:
- **Hotspot Profiler**: Samples function IPs using `perf_event_open` to find exact C-functions causing bottlenecks.
- **CPU Idle States**: Parses `/sys/devices/system/cpu/cpu*/cpuidle/` to measure C-State residency and detect blocking.
- **Hardware Cache Profiler**: Tracks `PERF_COUNT_HW_CACHE_MISSES` to calculate IPC and pinpoint memory stalls.
- **Energy Subsystem**: Integrates with Android Battery Management (`/sys/class/power_supply/battery`) and Intel RAPL to report exact Joules consumed.
- **Thermal Subsystem**: Scans Android thermal zones to detect severe CPU frequency throttling (>85°C).
- **SoftIRQ / Interrupts**: Snapshots `/proc/softirqs` to detect network/storage interrupt storms.
- **VMStat Thrashing**: Tracks `/proc/vmstat` major page faults to detect when the kernel starts aggressively swapping to `zRAM`.

---

```
# Expose kernel memory addresses to the profiler
echo 0 > /proc/sys/kernel/kptr_restrict

# Run the benchmark
./cbench -a -d 15 -j
```

### 3. Energy Subsystem on Android
Qualcomm Snapdragon and MediaTek Android devices rarely implement standard Linux power supply nodes. `cbench` dynamically scans `/sys/class/power_supply/` for `battery/` and `bms/` structures, looking for accumulative `energy_now` sensors. If none exist, it synthesizes total Joules by taking instantaneous `current_now` and `voltage_now` samples during the benchmark and integrating them over the time elapsed.
