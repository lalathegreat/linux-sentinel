# Linux Sentinel

### Predictive Failure Detection & Safe-Recovery Orchestrator

[![Language](https://img.shields.io/badge/Language-Modern%20C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20POSIX-orange.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Build Status](https://img.shields.io/badge/Build-Passing%20(CMake)-brightgreen.svg)]()
[![Tests](https://img.shields.io/badge/Tests-12%2F12%20Passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

**Linux Sentinel** is an individual, 100% software-based runtime reliability system written in Modern C++ (C++17) for Linux environments. It supervises mission-critical processes through non-invasive telemetry polling, statistical anomaly detection, DAG-based dependency tracking, pre-recovery state preservation (incident snapshots), bounded graceful restarts, and post-recovery verification.

Developed as a capstone project for **Wipro Embedded/Linux Systems Engineering**, strictly adhering to the 20-day individual project execution plan and 6-stage evaluation rubric.

---

## Deliverables & Documentation Matrix

| Document | Focus Area | Deliverable Document Link |
|:---:|---|---|
| **Work Log** | **20-Day Daily Engineering Work Log** | [docs/work-log.md](docs/work-log.md) |
| **Stage 1** | **Introduction & Problem Scope** | [docs/Stage1_Introduction.md](docs/Stage1_Introduction.md) |
| **Stage 2** | **Requirements & Acceptance Criteria** | [docs/PRD.md](docs/PRD.md) |
| **Stage 3** | **Architecture & UML Specifications** | [docs/Stage3_Architecture.md](docs/Stage3_Architecture.md) |
| **Stage 4** | **Prototype Implementation** | [src/](src/), [include/](include/), [config/](config/) |
| **Stage 5** | **Test Report & Verification Evidence (T1–T12)** | [docs/Stage5_TestReport.md](docs/Stage5_TestReport.md) |
| **Stage 6** | **Submission Bundle & Compliance Audit** | [docs/Stage6_SubmissionBundle.md](docs/Stage6_SubmissionBundle.md) |
| **Final Report** | **Comprehensive Technical Final Report** | [docs/FinalReport.md](docs/FinalReport.md) |
| **Presentation**| **10-Minute Presentation & Technical Defense** | [docs/Stage6_FinalPresentation.md](docs/Stage6_FinalPresentation.md) |

---

## Architectural Highlights

```text
                  +-----------------------------------+
                  |   Target Services (e.g. fault_app)|
                  +-----------------+-----------------+
                                    |
                    Telemetry via   |   POSIX Signals / Lifecycle
                    /proc & syscalls|   (SIGTERM -> SIGKILL, fork/exec)
                                    v
+-------------------------------------------------------------------------+
|                              LINUX SENTINEL                             |
|                                                                         |
|   1. CONFIG        2. COLLECT         3. DETECT         4. DIAGNOSE     |
|   [ConfigManager]->[HealthCollector]->[AnomalyDetector]->[DiagnosisEngine]|
|                                                                 |       |
|   8. AUDIT         7. CLOSE           6. VERIFY         5. RECOVER      |
|   [Logger]       <-[State Update]   <-[Verification]  <-[RecoveryMgr]   |
+-------------------------------------------------------------------------+
```

1. **Declarative Runtime Configuration (`ConfigManager`):** Parses `sentinel.conf` (INI format), validates numeric ranges, and fails safely on missing or malformed settings.
2. **Non-Invasive Telemetry Ingestion (`HealthCollector`):** Reads `/proc/[pid]/stat` and `/proc/[pid]/status` without kernel modification; computes differential CPU rate ($\Delta \text{ticks} / \Delta t$) across intervals.
3. **Multi-Sample Anomaly Detection (`AnomalyDetector`):** Sliding window ($N=5$) and rate-of-change ($\frac{\Delta \text{Mem}}{\Delta t} > 0$) detection over $K \ge 3$ consecutive breaches to eliminate single-spike false alarms.
4. **DAG Dependency Isolation (`DependencyManager`):** Directed Acyclic Graph tracking parent-child service relationships; prevents downstream restart storms when upstream root causes fail.
5. **Rule-Based Diagnosis (`DiagnosisEngine`):** Correlates telemetry symptoms with dependency health to produce classified root causes and recommended actions.
6. **Pre-Recovery Incident Snapshot (`CheckpointManager`):** Preserves immutable forensic snapshots (PID, telemetry, configuration, timestamp) to disk prior to any mutation.
7. **Bounded Tiered Recovery (`RecoveryManager`):** Enforces strict allowlists, circuit breaker retry limits (max 3 in 60s $\to$ `SAFE_MODE`), and two-tier signal escalation (`SIGTERM` grace window $\to$ `SIGKILL` fallback).
8. **Post-Recovery Health Verification (`VerificationEngine`):** Enforces a 2-second stabilization window to confirm new PID liveness and memory health.
9. **Incident Audit Logging (`Logger`):** Records structured incident cards (`INC-0001`) and audit trails to `logs/incidents.log`.

---

## Technical Decisions Settled Before Coding

- **Sampling Interval:** Default $1000\text{ms}$ ($1\text{s}$) polling interval balances real-time fault detection latency with negligible CPU overhead ($0.0\%$). Fully configurable between $50\text{ms}$ and $60000\text{ms}$ via `sentinel.conf`.
- **CPU Calculation Convention:** Process CPU accounting in Linux is cumulative via `utime` and `stime` ticks. Sentinel computes instantaneous utilization across two consecutive samples and elapsed wall-clock time divided by `sysconf(_SC_CLK_TCK)`. Percentage is reported per single-core convention ($0\% - 100\%$ nominal, capable of up to $N \times 100\%$ on multicore).
- **Memory Metric (RSS):** Monitored metric is **Resident Set Size (`VmRSS`)** in kilobytes (KB), representing actual physical RAM held by the process, rather than virtual address space (`VmSize`).
- **Persistence & Trend Analysis:** Requires 3 consecutive abnormal samples ($N=3$) before triggering an incident, avoiding false alerts from transient garbage-collection spikes. A consistent increase is categorized as `MEMORY_GROWTH` anomaly with evidentiary support.
- **Process Identity & Safety:** PID reuse is mitigated by verifying process start time and checking that the target process is explicitly allowlisted in configuration. Unregistered processes are never touched.
- **Safe Recovery Defaults:** Recovery defaults to "no action" when target state or identity is uncertain. Graceful stop (`SIGTERM` with 3000ms timeout) is mandatory before forceful `SIGKILL`.

---

## Quantitative Performance Benchmarks

| Metric | Target Specification | Measured Result | Status |
|---|:---:|:---:|:---:|
| **Resident Memory (RSS)** | $< 15.0\text{ MB}$ | **$2.18\text{ MB}$** | **$85.5\%$ below budget** |
| **CPU Utilization** | $< 1.5\%$ | **$0.0\% - 0.2\%$** | **Well within threshold** |
| **Recovery Latency** | $< 3.5\text{ seconds}$ | **$3.16\text{ seconds}$** | **Includes 2.0s post-restart stabilization** |
| **False Alarm Rate** | $0\%$ | **$0\%$** | **0 false alerts over steady runs** |
| **Memory Leak Cleanliness** | 0 leaks | **0 bytes leaked** | **Strict C++17 RAII discipline** |

---

## Risk Register & Guardrails

| Risk Identified | Root Cause / Threat | Prevention / Response Implemented in Code |
|---|---|---|
| **Scope Creep** | Unbounded requirements (ML, GUI, kernel drivers) | Scope strictly frozen to user-space C++17 daemon monitoring controlled test processes. |
| **False Positives** | Transient CPU or memory spikes triggering restarts | Multi-sample sliding window ($N=5$) requiring 3 consecutive breaches before action. |
| **Unsafe Process Actions** | Accidental termination of host services | Strict allowlisting in `sentinel.conf`; startup existence checks; refuse root privileges. |
| **Unclear Accounting** | Ambiguity between virtual vs physical memory | Explicit use of `VmRSS` in KB and tick deltas normalized by system clock ticks. |
| **Flapping / Restart Thrashing** | Repeatedly failing service causing crash loop | Circuit Breaker trips after 3 restarts in 60s, locking target into `SAFE_MODE`. |
| **Cascading Failures** | Restarting downstream apps when upstream is down | DAG dependency manager marks downstream blocked and suppresses restarts. |

---

## Quickstart & Verification

### Build from Source
```bash
# Clone the repository
git clone https://github.com/lalathegreat/linux-sentinel.git
cd linux-sentinel

# Configure and compile with CMake
cmake -B build -S .
cmake --build build
```

### Run Full Test Matrix (T1 through T12)
```bash
# Executes unit tests and all 12 test matrix scenarios with evidence capture
./tests/run_tests.sh
```

### Run Interactive Live Demos
```bash
# 1. Configuration-driven monitoring
./build/sentinel --config config/sentinel.conf

# 2. Memory leak self-healing demonstration
./build/sentinel --spawn ./build/fault_app --leak

# 3. Crash recovery demonstration
./build/sentinel --spawn ./build/fault_app --normal
```

---

## Final Submission Checklist

- [x] **Clean Build:** Builds cleanly from documented instructions using standard CMake & C++17 compiler (`g++` / `clang++`).
- [x] **Explicit Scope & Safety:** Strict user-space POSIX design; no kernel drivers, no root privileges required.
- [x] **Independent Code:** High-quality modular architecture; standard library only (zero external libraries).
- [x] **Staged Git Progress:** Clear semantic git commit history reflecting progressive milestones.
- [x] **Complete Test Matrix:** 12/12 automated test matrix scenarios (T1 to T12) passing with logs in `logs/`.
- [x] **Differentiated Logging:** Distinguishes symptom detection, recovery attempts, verified recovery, and circuit breaker trip.
- [x] **Honest Limitations:** Explicitly documented non-goals (no kernel driver, no black-box ML, heuristic diagnosis).
- [x] **Full Documentation Suite:** README, Work Log, PRD, Architecture, Test Report, Submission Bundle, Final Report, and Presentation Script complete.
- [x] **Clean GitHub Repository:** Public repository at [github.com/lalathegreat/linux-sentinel](https://github.com/lalathegreat/linux-sentinel) free of secrets or temporary files.
