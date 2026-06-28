# 📱 Android-Specific Profiling & Execution Notes

Android enforces an extremely strict SELinux and mount security model. To execute `cbench` and get accurate kernel profiling on an Android device, you must follow these exact steps:

## 1. Push to the Developer Temp Directory
Android mounts standard user partitions (like `/sdcard/`) with a `noexec` flag, which prevents binaries from running. You **must** push `cbench` directly to the shell developer directory.

From your host computer:
```bash
adb push cbench /data/local/tmp/
```

## 2. Disable Kernel Pointer Restrictions (`kptr_restrict`)
By default, Android's security policy sets `/proc/sys/kernel/kptr_restrict` to `2`. This forcibly hides all real memory addresses in `/proc/kallsyms`, causing the `cbench` profiler to throw `[unmapped_proprietary_code]` errors because it cannot map memory addresses to kernel symbols.

From your host computer, drop into the shell, escalate to root, fix the security constraints, and run the benchmark:

```bash
adb shell
su
cd /data/local/tmp/
chmod +x cbench

# Expose kernel memory addresses to the profiler
echo 0 > /proc/sys/kernel/kptr_restrict

# Run the benchmark
./cbench -a -d 15 -j
```

## 3. Energy Subsystem on Android
Qualcomm Snapdragon and MediaTek Android devices rarely implement standard Linux power supply nodes. `cbench` dynamically scans `/sys/class/power_supply/` for `battery/` and `bms/` structures, looking for accumulative `energy_now` sensors. If none exist, it synthesizes total Joules by taking instantaneous `current_now` and `voltage_now` samples during the benchmark and integrating them over the time elapsed.
