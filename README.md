# Linux Sentinel

### Predictive Failure Detection & Safe-Recovery Orchestrator

**Linux Sentinel** is an individual, 100% software-based runtime reliability system written in Modern C++ (C++17) for Linux environments. It monitors mission-critical processes through non-invasive telemetry polling, statistical anomaly detection, DAG-based dependency tracking, pre-recovery state preservation (incident snapshots), bounded graceful restarts, and post-recovery verification.

---

## Project Structure

```text
linux-sentinel/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # Project documentation & overview
├── .gitignore              # Build and environment exclusions
│
├── config/                 # Service definitions and failure thresholds
│   └── sentinel.conf
│
├── docs/                   # 6-Stage Capstone Deliverables
│   ├── PRD.md              # Product Requirements Document
│   ├── Stage1_Intro.md     # Stage 1: Problem statement & scope
│   ├── Stage2_Reqs.md      # Stage 2: Functional & non-functional specs
│   ├── Stage3_Arch.md      # Stage 3: Architecture & UML diagrams
│   ├── Stage5_Report.md    # Stage 5: Test report & metrics
│   └── Stage6_Slides.md    # Stage 6: 10-Minute Presentation Script
│
├── include/                # C++ Header files
│   ├── Common.h            # Core data structures (telemetry, incidents)
│   ├── HealthCollector.h   # /proc & syscall telemetry reader
│   ├── AnomalyDetector.h   # Sliding-window trend analyzer
│   ├── DependencyManager.h # DAG dependency resolver
│   ├── DiagnosisEngine.h   # Rule-based diagnostic classifier
│   ├── CheckpointManager.h # Pre-recovery forensic snapshot
│   ├── RecoveryManager.h   # POSIX signal & process restarter
│   ├── VerificationEngine.h# Post-recovery health validation
│   └── Logger.h            # Structured incident audit logger
│
├── src/                    # C++ Implementation files
│   ├── main.cpp            # Application entrypoint & event loop
│   └── ...
│
├── test_service/           # Synthetic target application
│   └── fault_app.cpp       # Simulates leaks, hangs, crashes for testing
│
├── tests/                  # Automated verification scripts
│   └── run_tests.sh
│
└── logs/                   # Audit trail & incident cards
    └── incidents.log
```

---

## Toolchain & Requirements

- **C++ Compiler:** `g++` / `clang++` supporting C++17 or C++20
- **Build System:** `cmake` (>= 3.16) and `make`
- **Version Control:** `git` (>= 2.25)
- **Target OS:** Linux (Ubuntu 20.04/22.04 LTS recommended; POSIX compatibility layer on macOS for development)

---

## Six-Stage Development Alignment

| Stage | Milestone | Primary Deliverable |
|---|---|---|
| **Stage 1** | Project Introduction | Problem Statement, Objectives, Boundaries |
| **Stage 2** | Requirements (PRD) | Functional/Non-functional specs, Acceptance criteria |
| **Stage 3** | Architecture & Design | Block Diagrams, UML, Data structures (`Common.h`) |
| **Stage 4** | Prototype Implementation | 6 incremental C++ prototype milestones |
| **Stage 5** | Testing & Evidence | 5 controlled fault scenarios, Test Report |
| **Stage 6** | Final Demonstration | 10-Minute live presentation & Q&A defense |
