# Contributing to Project Cerium Benchmarking

We welcome contributions to optimize kernel benchmarking utilities and the CAnalyze visualization dashboard. To maintain a high-quality codebase and clear change history, please adhere to the guidelines below.

---

## 🛠 Development Workflow

### Web Dashboard (CAnalyze)
CAnalyze is a Single Page Application (SPA) designed to compare JSON outputs from `cbench` runs.
- **Offline First**: All dependencies (e.g., Chart.js) must remain local and bundled. Do not reference external CDNs.
- **Run the local dev server**:
  - **Linux/macOS**: Run `./start_canalyze.sh`
  - **Windows**: Run `start_canalyze.bat`
- Navigate to `http://localhost:8080` to test changes.

### Core Benchmarking (`cbench`)
- The benchmarking suite is written in C.
- Maintain memory safety. Always audit pointer management to avoid segmentation faults or resource leaks.

---

## ✉️ Commit Message Guidelines

All commits must follow the **Linux Kernel commit message style**. Every logical change should be committed separately to form an incremental, easy-to-track patchset.

### Format
```text
subsystem: short imperative summary under 72 characters

A detailed explanation of the change, covering what was modified,
why the modification was necessary, and how it fixes the problem or
implements the feature.

Keep body paragraphs wrapped at 72 characters.

Signed-off-by: Your Name <your.email@example.com>
```

### Subsystems
Please prefix the commit subject with the appropriate subsystem:
- `canalyze:` — Web visualization frontend changes
- `bench:` — Core benchmarking engine (C code)
- `telemetry:` — Frequency, power, and state tracking
- `docs:` — Documentation and guides
- `ci:` — Github actions and build configs

### Checklist
1. **Imperative Mood**: Use imperative tone in the subject line (e.g., `canalyze: add radar zoom` instead of `Added radar zoom` or `Adds radar zoom`).
2. **Atomic Commits**: Separate UI layout, JS logic, and documentation changes into distinct commits.
3. **Signed-off-by**: Every commit must be signed off by the author (`git commit -s`).
