# Telemetry & OS Insights

Every benchmark in `cbench` is wrapped in a highly precise telemetry envelope that passively tracks kernel performance without altering the benchmark payload:

- **Hotspot Profiler**: Samples function IPs using `perf_event_open` to find exact C-functions causing bottlenecks.
- **CPU Idle States**: Parses `/sys/devices/system/cpu/cpu*/cpuidle/` to measure C-State residency and detect blocking.
- **Hardware Cache Profiler**: Tracks `PERF_COUNT_HW_CACHE_MISSES` to calculate IPC and pinpoint memory stalls.
- **Energy Subsystem**: Integrates with Android Battery Management (`/sys/class/power_supply/battery`) and Intel RAPL to report exact Joules consumed.
- **Thermal Subsystem**: Scans Android thermal zones to detect severe CPU frequency throttling (>85°C).
- **DVFS Tracking**: Tracks the weighted average CPU frequency during execution using `time_in_state` to expose lazy governors.
- **SoftIRQ / Interrupts**: Snapshots `/proc/softirqs` to detect network/storage interrupt storms.
- **VMStat Thrashing**: Tracks `/proc/vmstat` major page faults to detect when the kernel starts aggressively swapping to `zRAM`.
