# Cerium Benchmarking (`cbench`)

`cbench` is a highly specialized, statically compiled Linux kernel benchmarking and telemetry tool. Designed with zero external dependencies, it is explicitly built to stress-test core kernel subsystems, track hardware-level energy consumption, and automatically isolate algorithmic and architectural bottlenecks in the Linux Kernel.

## Features
- **Topology-Aware Threading**: Automatically detects CPU clusters (e.g., ARM `silver`/`gold` clusters) and binds threads appropriately for accurate scaling.
- **Hardware Energy Subsystem**: Integrates with Intel RAPL (`energy_uj`) and Android hardware sensors (Battery Management Systems, `current_now`, `energy_now`) to calculate operations-per-Joule efficiency.
- **Automated Bottleneck Analyzer**: Monitors CPU Idle Residency (`cpuidle` C-states vs. WFI) during benchmarks to definitively prove if a bottleneck is algorithmic (100% CPU usage) or due to severe hardware/lock contention (>90% idle time).
- **Native Kernel Profiler**: Uses `perf_event_open` and `/proc/kallsyms` to sample and pinpoint the exact C-functions dominating CPU time.
- **Symbolic Heuristics Engine**: Scans kernel profiler hotspots against known anti-patterns to provide direct patching advice (e.g., detecting `_raw_spin_lock` and suggesting RCU, or detecting `clear_page` and suggesting THP).
- **JSON Telemetry Export**: Dumps all metrics into a structured JSON file for CI/CD dashboard tracking.

## Benchmarks
1. **Syscall Latency (`SYS_gettid`)**: Measures raw context-switching overhead across the User/Kernel boundary.
2. **Scheduler Context Switching**: Measures inter-process communication and scheduling overhead via tightly coupled pipe ping-pongs.
3. **Memory Subsystem**: Aggregates throughput for sequential aligned writes and fast `memcpy` operations.
4. **I/O Subsystem**: Measures buffered write bandwidth (forcing `fsync` journal flushes) and cached read bandwidth.

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
- `-j` : Print structured JSON metrics at the end of the run.

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

# REQUIRED: Expose kernel memory addresses to the profiler
echo 0 > /proc/sys/kernel/kptr_restrict

# Run the benchmark
./cbench -a -d 15 -j
```

### 3. Energy Subsystem on Android
Qualcomm Snapdragon and MediaTek Android devices rarely implement standard Linux power supply nodes. `cbench` dynamically scans `/sys/class/power_supply/` for `battery/` and `bms/` structures, explicitly looking for accumulative `energy_now` sensors. If none exist, it synthesizes total Joules by taking instantaneous `current_now` and `voltage_now` samples during the benchmark and integrating them over the time elapsed.
