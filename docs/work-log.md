# Linux Sentinel — Engineering Work Log (20-Day Execution)

> **Project:** Linux Sentinel — Lightweight Early-Warning, Fault Diagnosis, and Safe Recovery for a Controlled Linux Test Process  
> **Repository:** `https://github.com/lalathegreat/linux-sentinel`  
> **Author:** Antigravity & Engineering Team  
> **Standards:** POSIX.1-2008 / C++17 | Wipro Embedded Systems Engineering  

---

## Daily Engineering Log (Days 1 – 20)

### Day 1 — Orient and Set Up
- **Date:** 2026-09-10
- **Task:** Read training brief and rubric. Set up development environment, verify compiler (`g++ -v`), `cmake`, `git`, and initialize Git repository.
- **Decisions:** Adopt modern C++17 with `-Wall -Wextra -Wpedantic` flags. Restrict dependencies entirely to standard POSIX user-space APIs (no third-party dependencies or external daemons).
- **Problems:** Ensuring cross-compilation and local testing compatibility across POSIX environments.
- **Fixes:** Structured CMake to use standard C++17 filesystem and POSIX headers.
- **Evidence:** Clean repository initialized; initial `.gitignore` and `CMakeLists.txt` committed (`7cb59ea`).

---

### Day 2 — Define the Problem
- **Date:** 2026-09-11
- **Task:** Formulate the formal problem statement, objective, target user persona, and expected outcome.
- **Decisions:** Constrain scope to single-node embedded Linux applications where silent process degradation (memory leaks, unhandled exits, thread stalls) leads to catastrophic kernel OOM killer panics or silent service blackout.
- **Evidence:** Authored [`docs/Stage1_Introduction.md`](file:///Users/maa/Desktop/Project/linux-sentinel/docs/Stage1_Introduction.md) (`21b0114`).

---

### Day 3 — Fix the Scope
- **Date:** 2026-09-12
- **Task:** Establish explicit scope boundaries and non-goals. Choose owned test program (`fault_app`) as monitored target.
- **Decisions:** Exclude kernel module/driver development, external cloud monitoring, and machine-learning models to adhere strictly to deterministic embedded user-space execution.
- **Evidence:** Added Boundary Matrix and Scope exclusions in Stage 1 documentation.

---

### Day 4 — Requirements Definition (PRD)
- **Date:** 2026-09-13
- **Task:** Define Functional Requirements (FR-1 to FR-10), Non-Functional Requirements (NFR-1 to NFR-5), and Acceptance Criteria (AC-01 to AC-06).
- **Decisions:** Assign strict IDs to each requirement and establish direct requirement-to-test mapping.
- **Evidence:** Authored [`docs/PRD.md`](file:///Users/maa/Desktop/Project/linux-sentinel/docs/PRD.md) (`6b0ac0f`).

---

### Day 5 — Architecture & Component Design
- **Date:** 2026-09-14
- **Task:** Design layered component diagram, data flow model, finite state machine (FSM), and sequence diagrams.
- **Decisions:** Adopt closed-loop event flow: `ConfigManager` $\to$ `HealthCollector` $\to$ `AnomalyDetector` $\to$ `DiagnosisEngine` $\to$ `RecoveryManager` $\to$ `VerificationEngine` $\to$ `Logger`.
- **Evidence:** Authored [`docs/Stage3_Architecture.md`](file:///Users/maa/Desktop/Project/linux-sentinel/docs/Stage3_Architecture.md) (`57feaf5`).

---

### Day 6 — Linux Process Study & Exploration
- **Date:** 2026-09-15
- **Task:** Study Linux process accounting via `/proc/[pid]/stat`, `/proc/[pid]/status`, POSIX signals (`SIGTERM`, `SIGKILL`, `SIGINT`), and PID reuse risks.
- **Decisions:** Use clock ticks (`sysconf(_SC_CLK_TCK)`) for delta CPU calculations; separate resident set size (`VmRSS`) from virtual memory (`VmSize`).
- **Evidence:** Linux concept notes recorded; `/proc` field parser architecture documented in architecture spec.

---

### Day 7 — C++ Data Structure Design
- **Date:** 2026-09-16
- **Task:** Define unified data structures in `Common.h`: `ProcessMetrics`, `AnomalyReport`, `DiagnosisRecord`, `IncidentCard`, `ServiceState`.
- **Decisions:** Avoid premature heap allocations in inner loops; use stack-allocated structs and enums with string converters.
- **Evidence:** Implemented [`include/Common.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/Common.h) (`370123b`).

---

### Day 8 — Build Skeleton & CLI Harness
- **Date:** 2026-09-17
- **Task:** Implement executable skeleton and signal handling for graceful shutdown on `SIGINT`/`SIGTERM`.
- **Decisions:** Thread-safe signal flags using `volatile sig_atomic_t`.
- **Evidence:** Implemented baseline `src/main.cpp` and verified clean compilation (`54df82a`).

---

### Day 9 — Process Identity & Safety Boundaries
- **Date:** 2026-09-18
- **Task:** Implement strict allowlist validation and PID identity verification to prevent PID reuse vulnerabilities.
- **Decisions:** Sentinel will refuse to touch or signal any PID that does not match the configured service name or is missing at startup.
- **Evidence:** Added safety checks in `RecoveryManager` and startup validator in `main.cpp`.

---

### Day 10 — Health Collection Subsystem
- **Date:** 2026-09-19
- **Task:** Implement `HealthCollector` to parse `/proc/[pid]/stat` and `/proc/[pid]/status` without external subprocess overhead.
- **Problems:** Zombie processes (`state_char == 'Z'`) still return 0 for `kill(pid, 0)`. Furthermore, calling `waitpid` on non-child processes returns `-1` with `errno == ECHILD`.
- **Fixes:** Differentiated direct children from external processes: only treat as exited if `waitpid == pid`. For non-child processes, fall through to `kill(pid, 0)`.
- **Evidence:** Implemented [`include/HealthCollector.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/HealthCollector.h) and [`src/HealthCollector.cpp`](file:///Users/maa/Desktop/Project/linux-sentinel/src/HealthCollector.cpp) (`24e56ea`).

---

### Day 11 — Anomaly Detection Rules & Sliding Window
- **Date:** 2026-09-20
- **Task:** Implement `AnomalyDetector` with sliding history window ($W=5$), monotonic slope detection, and consecutive violation filtering ($N=3$).
- **Decisions:** Distinguish memory leaks (positive monotonic slope) from isolated temporary spikes.
- **Evidence:** Implemented [`include/AnomalyDetector.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/AnomalyDetector.h) and [`src/AnomalyDetector.cpp`](file:///Users/maa/Desktop/Project/linux-sentinel/src/AnomalyDetector.cpp) (`899254a`).

---

### Day 12 — Diagnosis Engine & Incident Attribution
- **Date:** 2026-09-21
- **Task:** Implement rule-based diagnosis engine that correlates anomalies against DAG dependencies and assigns cautious symptom labels.
- **Decisions:** If an upstream dependency is down, suppress downstream restarts and assign `DEPENDENCY_FAILURE` with 0 confidence for downstream fault.
- **Evidence:** Implemented [`include/DiagnosisEngine.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/DiagnosisEngine.h) and [`src/DiagnosisEngine.cpp`](file:///Users/maa/Desktop/Project/linux-sentinel/src/DiagnosisEngine.cpp) (`6c7079f`).

---

### Day 13 — Safe Recovery State Machine & Circuit Breaker Design
- **Date:** 2026-09-22
- **Task:** Design finite state machine for recovery: `OBSERVE` $\to$ `DIAGNOSE` $\to$ `CHECKPOINT` $\to$ `RECOVER` $\to$ `VERIFY`. Design 3-strike circuit breaker.
- **Decisions:** Cap auto-restarts at 3 within a sliding 60-second window. Transition permanently to `SAFE_MODE` upon 4th breach.
- **Evidence:** Recovery state machine documented in architecture spec.

---

### Day 14 — Controlled Recovery Execution
- **Date:** 2026-09-23
- **Task:** Implement `RecoveryManager` with two-phase signal escalation: `SIGTERM` with 3000ms timeout $\to$ `SIGKILL` fallback.
- **Decisions:** Use non-blocking `waitpid(pid, &status, WNOHANG)` loops and fork/execvp respawn.
- **Evidence:** Implemented [`include/RecoveryManager.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/RecoveryManager.h) and [`src/RecoveryManager.cpp`](file:///Users/maa/Desktop/Project/linux-sentinel/src/RecoveryManager.cpp) (`fd7b0aa`).

---

### Day 15 — Post-Recovery Verification & Forensics
- **Date:** 2026-09-24
- **Task:** Implement `VerificationEngine` and `CheckpointManager`. Record pre-recovery state and verify post-relaunch stability over a 2000ms observation window.
- **Decisions:** Distinguish attempted recovery from verified recovery. Format human-readable Incident Cards (`INC-xxxx`).
- **Evidence:** Implemented [`include/VerificationEngine.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/VerificationEngine.h) and [`include/CheckpointManager.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/CheckpointManager.h).

---

### Day 16 — Controlled Fault Injection Harness
- **Date:** 2026-09-25
- **Task:** Create `test_service/fault_app.cpp` to deterministically simulate normal baseline, progressive memory leak, high CPU load, and crash-on-start.
- **Evidence:** Implemented [`test_service/fault_app.cpp`](file:///Users/maa/Desktop/Project/linux-sentinel/test_service/fault_app.cpp) (`54df82a`).

---

### Day 17 — Hardening & Edge-Case Testing
- **Date:** 2026-09-26
- **Task:** Implement `ConfigManager` (`sentinel.conf` INI parser) and harden edge cases: malformed config, missing files, absent target PID at startup, unwritable log paths, and process disappearing mid-read.
- **Problems:** Subprocess arguments starting with `-` (e.g. `--leak`) were prematurely terminating argument loops in `main.cpp`.
- **Fixes:** Updated argument parsing to only break on known Sentinel flags (`-c`, `-d`, `-m`, `-s`).
- **Evidence:** Implemented `ConfigManager` and unit test `test_config.cpp`.

---

### Day 18 — Full Test Matrix Execution (T1 to T12)
- **Date:** 2026-09-27
- **Task:** Expand test runner `run_tests.sh` to execute all 12 test matrix scenarios (T1 to T12) with full evidence capture and zero regressions.
- **Evidence:** 12/12 tests passing; benchmark recorded: 2.18 MB RSS, 0.0% CPU (`97edbeb`).

---

### Day 19 — Packaging, Documentation & Submission Bundle
- **Date:** 2026-09-28
- **Task:** Consolidate all 6 curriculum stages into the final deliverable matrix, verify clean build from fresh clone, and write final documentation.
- **Evidence:** Authored [`docs/Stage6_SubmissionBundle.md`](file:///Users/maa/Desktop/Project/linux-sentinel/docs/Stage6_SubmissionBundle.md) (`e855fb0`).

---

### Day 20 — Final Presentation & Defense Rehearsal
- **Date:** 2026-09-29
- **Task:** Prepare 10-minute presentation deck script, live terminal demo playbook, and 8 deep-dive technical panel Q&A defenses.
- **Evidence:** Authored [`docs/Stage6_FinalPresentation.md`](file:///Users/maa/Desktop/Project/linux-sentinel/docs/Stage6_FinalPresentation.md) (`2755b9f`).
