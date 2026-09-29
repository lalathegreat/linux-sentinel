# Stage 1 — Project Introduction & Problem Definition

**Project Name:** Linux Sentinel: Predictive Failure Detection & Safe-Recovery Orchestrator  
**Domain:** Embedded Linux / Systems Programming / Reliability Engineering  
**Implementation Language:** Modern C++ (C++17)  
**Target Environment:** Linux (Ubuntu 20.04/22.04 LTS / POSIX-compliant user space)  
**Developer:** Individual Project (Wipro Embedded/Linux Capstone)  

---

## 1. Problem Statement

Embedded Linux appliances (such as automotive gateways, medical devices, edge gateways, and industrial controllers) operate autonomously without direct, continuous human intervention. In these long-running environments, background services frequently encounter progressive runtime degradation—including gradual memory leaks, runaway thread spin-loops, deadlocks, and silent resource exhaustion. 

Traditional Linux monitoring approaches suffer from severe shortcomings:
1. **Passive Dashboards:** Existing utilities (e.g., standard `top`, `ps`, or basic scripts) merely display numbers to human operators rather than autonomously intervening before failures cascade.
2. **Delayed Kernel Reaction:** Default operating system mechanisms intervene only after catastrophic damage has already occurred (such as the Linux kernel Out-Of-Memory (OOM) killer terminating critical tasks unpredictably).
3. **Crude Reboots & Flapping:** Rudimentary watchdog scripts attempt uncoordinated, immediate restarts or system reboots. This risks file system corruption, triggers restart storms (infinite reboot loops), and ignores upstream service dependencies.

There is a critical need for a **lightweight, user-space reliability orchestrator** that continuously tracks resource trends, correlates dependencies, creates pre-recovery forensic snapshots, safely recovers unhealthy services with strict retry bounds, and verifies post-recovery health before closing incidents.

---

## 2. Core Project Objective (Formal 3–5 Sentences)

> **"Linux Sentinel is a software-based Linux runtime reliability system developed in Modern C++ that supervises registered mission-critical services without physical hardware dependencies. It non-invasively polls Linux system telemetry through `/proc` interfaces, detects early behavioral anomalies using multi-sample trend analysis, and correlates faults against a service dependency graph. When degradation or failure occurs, Sentinel preserves pre-recovery forensic state in an incident snapshot, orchestrates a bounded graceful recovery sequence with circuit-breaker protection, and verifies runtime stabilization. The final live demonstration definitively proves that an autonomous Linux service exhibiting simulated progressive memory exhaustion or an unexpected crash can be safely detected, diagnosed, recovered, verified healthy, and audited in under 3 seconds without human intervention."**

---

## 3. High-Level Concept: The Closed-Loop Reliability Pipeline

Linux Sentinel shifts system monitoring from passive observation to an active, closed-loop feedback control cycle:

```text
                  +-----------------------------------+
                  |   Target Services (e.g. fault_app)|
                  +-----------------+-----------------+
                                    |
                    Telemetry via   |   Signals / Process Lifecycle
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

---

## 4. System Scope & Engineering Boundaries

To guarantee an achievable, robust, and defensible project for a 10-minute technical evaluation, the system boundaries are strictly defined:

| Capability Category | What Linux Sentinel DOES Build | What Linux Sentinel DOES NOT Build |
|---|---|---|
| **Operating Space** | **100% User-Space Linux:** Runs as a standard system reliability daemon using POSIX system calls. | **No Custom Kernel / Bootloader:** Does not modify kernel source, build a bootloader, or compile a new OS. |
| **Hardware Scope** | **100% Software-Only:** Works on standard Linux virtual machines, PCs, and workstations. | **No Physical Hardware:** No Arduino, Raspberry Pi, physical sensors, or external microcontrollers. |
| **Process Supervision** | **Targeted Allowlist:** Supervises its own registered target processes (e.g., `fault_app`). | **No Arbitrary System Takeover:** Does not restart root system daemons (e.g., `systemd`, `init`). |
| **Diagnostic Model** | **Deterministic Rule-Based Engine:** Sliding windows, rate of change ($\frac{\Delta \text{Mem}}{\Delta t}$), and DAG analysis. | **No Heavy ML / AI Models:** Avoids non-deterministic black-box machine learning models or cloud inference. |
| **State Preservation** | **Forensic Incident Snapshot:** Preserves PID metadata, telemetry, configuration, and timestamps. | **No Full OS Disk Rollback:** Does not snapshot or restore entire disk images or virtual machine disks. |
| **User Interface** | **Clean ANSI Terminal Interface (TUI):** Real-time CLI status board and structured audit logs. | **No Web / Cloud Dashboard:** No React, Node.js, WebSockets, or external database infrastructure. |

---

## 5. What the Final Demonstration Will Prove

During the final 10-minute capstone evaluation, the project will prove the following live in the terminal:

1. **Baseline Reliability (TC-01):** Sentinel monitors a healthy service with negligible CPU overhead ($< 1.0\%$) and zero false alerts.
2. **Predictive Memory Leak Interception (TC-03):** When a controlled memory leak is triggered in the target service, Sentinel identifies the continuous upward slope across consecutive samples, diagnoses `OOM_RISK` *before* the Linux kernel OOM killer fires, captures an incident snapshot, executes a graceful restart, and verifies recovery.
3. **Sudden Crash Recovery (TC-02):** When a target is abruptly killed (`SIGKILL`), Sentinel detects process termination within 1 second, restarts the process, and verifies the new PID.
4. **Circuit Breaker / Safe Mode (TC-04):** When a malfunctioning service repeatedly fails upon startup, Sentinel halts recovery attempts after 3 retries, transitions the service to `SAFE_MODE`, and protects the host system against restart storms.
5. **Auditable Incident Reporting:** A structured forensic incident report (e.g., `INC-0001`) is generated with precise detection latencies, root cause diagnosis, recovery actions, and verification results.

---

## 6. Alignment with Wipro 6-Stage Rubric

- **Stage 1 (Current):** Problem Statement, Objectives, Scope, Expected Outcome (`docs/Stage1_Introduction.md`).
- **Stage 2:** Product Requirements Document & Acceptance Criteria (`docs/PRD.md`).
- **Stage 3:** Architecture, UML Diagrams, Data Structures (`docs/Stage3_Architecture.md`, `include/Common.h`).
- **Stage 4:** Modular Prototype Implementation across 6 Milestones.
- **Stage 5:** Controlled Fault Injection Testing & Test Report (`docs/Stage5_TestReport.md`).
- **Stage 6:** Final 10-Minute Presentation Deck & Live Demo (`docs/Stage6_FinalPresentation.md`).
