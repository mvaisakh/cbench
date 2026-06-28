# Supported Benchmarks

`cbench` includes several specialized benchmarks designed to isolate specific kernel subsystems.

1. **Syscall Latency (`SYS_gettid`)**: Measures raw context-switching overhead across the User/Kernel boundary.
2. **Scheduler Context Switching**: Measures inter-process communication and scheduling overhead via tightly coupled pipe ping-pongs.
3. **Memory Subsystem**: Aggregates throughput for sequential aligned writes and fast `memcpy` operations.
4. **I/O Subsystem**: Measures buffered write bandwidth (forcing `fsync` journal flushes) and cached read bandwidth.
5. **RNG Subsystem**: Threads constantly `read()` from `/dev/urandom` to stress the kernel entropy pool, ChaCha20 PRNG, and `drivers/char/random.c` locks.
6. **Network Stack**: UDP loopback (`127.0.0.1`) blasting to stress `sk_buff` allocation, network namespaces, and the TCP/IP stack (`net/core/dev.c`).
7. **Futex Contention**: Threads heavily contest a shared futex via `syscall(SYS_futex)`. This is the #1 bottleneck for the Android Java Runtime (ART).
8. **Kernel Crypto API (`AF_ALG`)**: Binds to `AF_ALG` sockets to compute `sha256` hashes, testing if the kernel is leveraging hardware cryptography (like Qualcomm CE).
9. **Zero / Copy-to-User**: Massive concurrent `read()`s from `/dev/zero` to isolate and stress `clear_page` and `copy_to_user` memory operations.
10. **NEON Thermal Throttling**: Massive SIMD matrix multiplications to generate maximum heat and test the kernel's thermal frequency scaling limits.
11. **SQLite WAL Emulation**: Rapid 4KB `pwrite()` and `fdatasync()` calls to test VFS journaling efficiency (F2FS vs EXT4).
12. **ZRAM Compression Stress**: Forces `kswapd` thrashing by allocating large, sparse buffers with alternating compressible and uncompressible pages.
13. **EAS Ping-Pong**: Tests Energy Aware Scheduling (EAS) by measuring the condition-variable wakeup latency between a LITTLE core and a big/Prime core.
