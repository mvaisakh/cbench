# Cerium Benchmarking (`cbench`)

`cbench` is a highly specialized, statically compiled Linux kernel benchmarking and telemetry tool. Designed with zero external dependencies, it is explicitly built to stress-test core kernel subsystems, track hardware-level energy consumption, and automatically isolate algorithmic and architectural bottlenecks in the Linux Kernel.

## 📖 Documentation Directory
For in-depth details, please refer to the specific documentation files:
- [Supported Benchmarks](docs/BENCHMARKS.md): Details on the 13 built-in kernel benchmarks (NEON, Futex, RCU, ZRAM, etc.)
- [Telemetry & OS Insights](docs/TELEMETRY.md): Details on the profiler, thermal, DVFS, and energy tracking systems.
- [Android Execution Guide](docs/ANDROID.md): Required steps for bypassing Android `noexec` mounts and `kptr_restrict` security.
- [CAnalyze Visualizer](docs/CANALYZE.md): Guide to using the built-in Material Design web dashboard for comparing benchmark runs.

---

## 🚀 Quick Start

You must run `cbench` as `root` (`sudo` or `su`) so that it can access `/sys` topology nodes, Intel/Android power sensors, and the `perf_event_open` kernel profiler rings!

### Standard Linux (x86_64 / ARM64)
Ensure you have the static C libraries installed on your host system (e.g., `glibc-static` on Fedora/RHEL).
```bash
make clean && make
sudo ./cbench -a -d 15 -j
```

### Android (Cross-Compilation via NDK)
Android does not use standard `glibc`. To cross-compile `cbench` for Android, use the prebuilt Clang compiler provided by the Android NDK:
```bash
make clean
CC=/path/to/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android33-clang make
```
*(For detailed steps on executing this on an Android device, see the [Android Execution Guide](docs/ANDROID.md))*

---

## 📊 Visual Analysis (CAnalyze)
Compare your "Before" and "After" JSON outputs visually using the included CAnalyze web application:
```bash
./start_canalyze.sh
```
Navigate to `http://localhost:8080` in your browser to view the interactive Performance Radar Charts.
