# Product Requirements Document (PRD)

**Project Name:** Linux Sentinel: Predictive Failure Detection & Safe-Recovery Orchestrator  
**Version:** 1.0.0-PROD  
**Target Platform:** Embedded Linux (Ubuntu 20.04/22.04 LTS / POSIX User-Space)  
**Implementation Standard:** Modern C++ (C++17)  
**Stage Alignment:** Stage 2 — Requirements & Specifications  

---

## 1. Product Vision & Operational Context

Linux Sentinel is a lightweight, closed-loop runtime reliability supervisor designed for mission-critical embedded Linux environments. Unlike passive monitoring tools that require human intervention or blunt kernel-level crash handlers, Sentinel continuously monitors registered user-space processes, detects behavioral degradation (such as memory leaks and thread stalls) using multi-sample trend analysis, captures pre-mutation forensic snapshots, executes bounded recovery actions, and verifies stabilization.

### 1.1 Target Users & Operational Scenarios
- **Embedded Systems Engineers:** Requiring autonomous self-healing for remote gateways and headless systems.
- **Wipro Interview Evaluation Panel:** Verifying deep proficiency in Linux system calls, process lifecycles, POSIX signals, modern C++ architecture, and reliability engineering.

---

## 2. Functional Requirements (FR)

| Requirement ID | Module / Feature | Priority | Specification & Behavior |
|---|---|:---:|---|
| **FR-1** | **Service Allowlist & Registration** | **P0** | Sentinel shall read a declarative configuration file (`config/sentinel.conf`) on startup. It shall register only explicit target services by name, executable path, start arguments, and health probe criteria. Unregistered processes shall never be monitored or signaled. |
| **FR-2** | **Non-Invasive Telemetry Polling** | **P0** | Sentinel shall extract runtime process metrics directly from Linux virtual filesystem interfaces (`/proc/[pid]/stat`, `/proc/[pid]/status`) and system calls (`sysinfo()`, `kill(pid, 0)`). It shall compute: (a) differential CPU percentage across interval $\Delta t$, (b) Resident Set Size (RSS in KB/MB), (c) Virtual Memory Size (VmSize), and (d) process state (`R`, `S`, `Z`, `D`, `T`). |
| **FR-3** | **Multi-Sample Trend & Anomaly Detection** | **P0** | Sentinel shall maintain a circular sliding window of historical telemetry samples ($N=5$). To prevent false alarms from transient spikes, a fault condition shall only trigger when $K \ge 3$ consecutive samples breach configured thresholds or show a sustained monotonic upward slope ($\frac{\Delta \text{Mem}}{\Delta t} > 0$). |
| **FR-4** | **DAG Dependency Management** | **P0** | Sentinel shall maintain an in-memory Directed Acyclic Graph (DAG) representing service relationships (e.g., `application` depends on `backend`, which depends on `database`). If an upstream dependency fails, Sentinel shall mark downstream services as `BLOCKED` rather than repeatedly attempting invalid downstream restarts. |
| **FR-5** | **Rule-Based Diagnostic Classification** | **P0** | Sentinel shall feed telemetry trends and dependency states into a deterministic rule engine to classify anomalies into distinct fault categories: `PROCESS_TERMINATED`, `MEMORY_LEAK_WARNING`, `CPU_STARVATION`, or `UPSTREAM_DEPENDENCY_FAULT`. |
| **FR-6** | **Pre-Recovery Incident Snapshot** | **P0** | Prior to executing any disruptive recovery action, Sentinel shall capture a forensic checkpoint preserving: target PID, command-line arguments, telemetry at failure time, active configuration version, failure reason, and high-resolution timestamp. |
| **FR-7** | **Bounded Tiered Recovery Orchestration** | **P0** | Sentinel shall execute a controlled two-tier termination sequence: <br>1. *Tier 1 (Graceful):* Send `SIGTERM` (signal 15) and await process exit for a configurable grace period (default: 3 seconds) using `waitpid(WNOHANG)`.<br>2. *Tier 2 (Forceful Fallback):* If process fails to exit after grace period, send `SIGKILL` (signal 9).<br>3. *Respawn:* Re-launch target process using `fork()` and `execvp()`, recording the newly assigned PID. |
| **FR-8** | **Circuit Breaker / Flapping Suppression** | **P0** | Sentinel shall enforce a retry limit (maximum 3 recovery attempts within a rolling 60-second window). If the limit is exceeded, Sentinel shall cease auto-recovery, transition the target to `SAFE_MODE`, and log a critical administrative alert to prevent restart thrashing. |
| **FR-9** | **Post-Recovery Health Verification** | **P0** | Sentinel shall not assume recovery succeeded solely because `execvp()` executed. It shall monitor the new PID across a stabilization window (default: 2 seconds) to verify that the PID is alive, process state is `RUNNING` or `SLEEPING`, and initial resource consumption is within normal baseline. |
| **FR-10** | **Incident Audit Logging** | **P0** | Sentinel shall record all detection events, diagnoses, checkpoints, recovery attempts, and verification results in both human-readable format and structured JSON cards (`logs/incidents.log`). |

---

## 3. Non-Functional Requirements (NFR)

### 3.1 NFR-1: Resource Efficiency & Low Overhead
- **CPU Utilization:** Sentinel itself shall consume less than **1.5% CPU** on a single core during active polling.
- **Memory Footprint:** Sentinel's Resident Set Size (RSS) shall not exceed **15 MB**.
- **Polling Frequency:** Configurable interval between **500 ms and 2000 ms** (default: 1000 ms).

### 3.2 NFR-2: Safety & Process Isolation
- Sentinel runs entirely in **Linux user space**.
- Sentinel shall strictly enforce allowlist boundaries: under no circumstances shall it emit signals (`SIGTERM`, `SIGKILL`) or attempt restarts on unregistered PIDs or host system daemons.
- PID validation: Before sending any signal, Sentinel shall confirm the process identity to eliminate PID recycling/reuse race conditions.

### 3.3 NFR-3: Reliability & Bounded State Execution
- The supervisor must remain stable even if the monitored test process crashes violently, hangs in an uninterruptible sleep state, or floods standard output.
- All dynamic memory in C++ shall follow strict **RAII (Resource Acquisition Is Initialization)** principles using smart pointers (`std::unique_ptr`, `std::shared_ptr`) with zero memory leaks detectable by Valgrind.

### 3.4 NFR-4: Auditability & Human-Readable Output
- Log entries must contain ISO 8601 millisecond timestamps.
- Terminal user interface (TUI) shall provide a clear, ANSI-colored summary table indicating: Service Name, PID, CPU %, RSS Memory, Health Status, and Active Incident Count.

### 3.5 NFR-5: Portability & Standard Compliance
- Built using **C++17** conforming to standard ISO/IEC 14882:2017.
- POSIX-compliant system calls conforming to IEEE Std 1003.1-2008.
- Builds cleanly using standard CMake (version $\ge 3.16$) and GCC/Clang with zero compilation warnings (`-Wall -Wextra -Werror`).

---

## 4. Measurable Acceptance Criteria (UAT Matrix)

| Test ID | Test Scenario | Input / Stimulus | Expected Behavior | Acceptance Threshold |
|---|---|---|---|:---:|
| **AC-01** | **Normal Operation Baseline** | Start `fault_app --normal` | Sentinel polls telemetry every 1s; status reports `HEALTHY`; zero false alarms. | 0 false alerts over 60s run |
| **AC-02** | **Crash Detection & Respawn** | Send `kill -9` to target PID | Sentinel detects missing PID within 1 monitoring tick ($< 1.0\text{s}$), captures snapshot, respawns process, and updates PID. | Recovery completed in $< 2.5\text{s}$ |
| **AC-03** | **Proactive Memory Leak Interception** | Run `fault_app --leak` (+5MB/s) | Sentinel identifies monotonic growth across 3 samples, diagnoses `MEMORY_LEAK_WARNING`, captures snapshot, triggers graceful restart, and verifies baseline memory. | Action taken before system OOM kill |
| **AC-04** | **Circuit Breaker Engagement** | Run `fault_app --crash-on-start` | Target crashes on launch 3 times; Sentinel aborts restart after 3rd attempt, logs `RECOVERY_LIMIT_EXCEEDED`, sets state to `SAFE_MODE`. | Zero further restarts; system protected |
| **AC-05** | **Graceful vs Forceful Escalation** | Target ignores `SIGTERM` | Sentinel issues `SIGTERM`, waits 3s grace timeout, detects PID still alive, escalates to `SIGKILL`, reaps zombie process, and respawns. | Successful cleanup within 4.0s total |
| **AC-06** | **Incident Report Completeness** | Any incident triggered | `logs/incidents.log` contains complete incident card with unique ID, timestamps, failure cause, actions taken, and PASS/FAIL result. | 100% field completeness in log |

---

## 5. System Constraints & Assumptions

1. **Host Environment:** Target deployment is standard 64-bit Linux (x86_64 or aarch64) with the `/proc` pseudo-filesystem mounted and accessible.
2. **Permissions:** Sentinel operates under standard user permissions, supervising child processes owned by the same user account, eliminating the need for elevated `sudo` privileges during testing.
3. **No External Dependencies:** Sentinel relies purely on standard C++ STL and POSIX APIs; it requires no third-party libraries (no Boost, no Qt, no external web servers).
