# Linux Sentinel

### Predictive Failure Detection & Safe-Recovery Orchestrator

[![Language](https://img.shields.io/badge/Language-Modern%20C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20POSIX-orange.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Build Status](https://img.shields.io/badge/Build-Passing%20(CMake)-brightgreen.svg)]()
[![Tests](https://img.shields.io/badge/Tests-5%2F5%20Passing-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

**Linux Sentinel** is an individual, 100% software-based runtime reliability system written in Modern C++ (C++17) for Linux environments. It supervises mission-critical processes through non-invasive telemetry polling, statistical anomaly detection, DAG-based dependency tracking, pre-recovery state preservation (incident snapshots), bounded graceful restarts, and post-recovery verification.

Developed as a capstone project for **Wipro Embedded/Linux Systems Engineering**, strictly adhering to the 6-stage evaluation rubric.

---

## Six-Stage Curriculum Deliverables

| Curriculum Stage | Focus Area | Deliverable Document Link |
|:---:|---|---|
| **Stage 1** | **Introduction & Problem Scope** | [docs/Stage1_Introduction.md](docs/Stage1_Introduction.md) |
| **Stage 2** | **Requirements & Acceptance Criteria** | [docs/PRD.md](docs/PRD.md) |
| **Stage 3** | **Architecture & UML Specifications** | [docs/Stage3_Architecture.md](docs/Stage3_Architecture.md) |
| **Stage 4** | **Prototype Implementation** | [src/](src/) & [include/](include/) |
| **Stage 5** | **Test Report & Verification Evidence** | [docs/Stage5_TestReport.md](docs/Stage5_TestReport.md) |
| **Stage 6** | **Submission Bundle & Compliance Audit** | [docs/Stage6_SubmissionBundle.md](docs/Stage6_SubmissionBundle.md) |

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
|   1. COLLECT       2. DETECT          3. DIAGNOSE       4. CHECKPOINT   |
|   [HealthCollector]->[AnomalyDetector]->[DiagnosisEngine]->[Snapshot]   |
|                                                                 |       |
|   8. AUDIT         7. CLOSE           6. VERIFY         5. RECOVER      |
|   [Logger]       <-[State Update]   <-[Verification]  <-[RecoveryMgr]   |
+-------------------------------------------------------------------------+
```

1. **Non-Invasive Telemetry Ingestion (`HealthCollector`):** Reads `/proc/[pid]/stat` and `/proc/[pid]/status` without kernel modification; computes differential CPU rate ($\Delta \text{ticks} / \Delta t$) across intervals.
2. **Multi-Sample Anomaly Detection (`AnomalyDetector`):** Sliding window ($N=5$) and rate-of-change ($\frac{\Delta \text{Mem}}{\Delta t} > 0$) detection over $K \ge 3$ consecutive breaches to eliminate single-spike false alarms.
3. **DAG Dependency Isolation (`DependencyManager`):** Directed Acyclic Graph tracking parent-child service relationships; prevents downstream restart storms when upstream root causes fail.
4. **Rule-Based Diagnosis (`DiagnosisEngine`):** Correlates telemetry symptoms with dependency health to produce classified root causes and recommended actions.
5. **Pre-Recovery Incident Snapshot (`CheckpointManager`):** Preserves immutable forensic snapshots (PID, telemetry, configuration, timestamp) to disk prior to any mutation.
6. **Bounded Tiered Recovery (`RecoveryManager`):** Enforces strict allowlists, circuit breaker retry limits (max 3 in 60s $\to$ `SAFE_MODE`), and two-tier signal escalation (`SIGTERM` grace window $\to$ `SIGKILL` fallback).
7. **Post-Recovery Health Verification (`VerificationEngine`):** Enforces a 2-second stabilization window to confirm new PID liveness and memory health.
8. **Incident Audit Logging (`Logger`):** Records structured incident cards (`INC-0001`) and audit trails to `logs/incidents.log`.

---

## Quantitative Performance Benchmarks

| Metric | Target Specification | Measured Result | Status |
|---|:---:|:---:|:---:|
| **Resident Memory (RSS)** | $< 15.0\text{ MB}$ | **$2.18\text{ MB}$** | **$85.5\%$ below budget** |
| **CPU Utilization** | $< 1.5\%$ | **$0.0\% - 0.2\%$** | **Well within threshold** |
| **Recovery Latency** | $< 3.5\text{ seconds}$ | **$2.01\text{ seconds}$** | **Includes 2.0s stabilization** |
| **False Alarm Rate** | $0\%$ | **$0\%$** | **0 false alerts over steady runs** |
| **Memory Leak Cleanliness** | 0 leaks (Valgrind-clean) | **0 bytes leaked** | **Strict C++17 RAII discipline** |

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

### Run Automated Test Suite
```bash
# Executes unit tests, 5 fault injection scenarios (TC-01 to TC-05), and gathers benchmarks
./tests/run_tests.sh
```

### Run Interactive Live Demo
```bash
# Supervise the synthetic target under simulated progressive memory leakage
./build/sentinel --spawn ./build/fault_app --leak
```
