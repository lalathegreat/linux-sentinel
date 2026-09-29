# Stage 3 — System Architecture & Design Specification

**Project Name:** Linux Sentinel: Predictive Failure Detection & Safe-Recovery Orchestrator  
**Version:** 1.0.0-PROD  
**Target Platform:** Embedded Linux (POSIX User-Space)  
**Implementation Standard:** Modern C++ (C++17)  
**Stage Alignment:** Stage 3 — Architecture, UML & Data Structures  

---

## 1. System Block Diagram & Subsystem Boundaries

Linux Sentinel is partitioned into decoupled subsystems, each adhering to the Single Responsibility Principle (SRP). Communication between modules is mediated by strongly typed data contracts defined in [`include/Common.h`](file:///Users/maa/Desktop/Project/linux-sentinel/include/Common.h).

```mermaid
flowchart TD
    subgraph ConfigLayer ["Configuration Layer"]
        CFG["ConfigManager\n(sentinel.conf)"]
    end

    subgraph TargetLayer ["Supervised Target Layer"]
        PROC["Registered Target Services\n(e.g., fault_app)"]
    end

    subgraph ObservabilityLayer ["Observability & Ingestion Layer"]
        HC["HealthCollector\n(Reads /proc/[pid]/stat & status)"]
    end

    subgraph IntelligenceLayer ["Diagnostic & Intelligence Layer"]
        AD["AnomalyDetector\n(Sliding Window & Trend Slope)"]
        DM["DependencyManager\n(DAG Dependency Tree)"]
        DE["DiagnosisEngine\n(Rule-Based Correlation)"]
    end

    subgraph ControlLayer ["Actuation & Control Layer"]
        CP["CheckpointManager\n(Incident Snapshot)"]
        RM["RecoveryManager\n(SIGTERM / SIGKILL Escalator & Circuit Breaker)"]
        VE["VerificationEngine\n(PID & Health Prober)"]
    end

    subgraph AuditLayer ["Audit & Presentation Layer"]
        LOG["Logger\n(incidents.log & Audit Trail)"]
        TUI["SentinelTUI\n(ANSI Status Dashboard)"]
    end

    CFG --> HC
    CFG --> DM
    CFG --> RM
    PROC -.->|/proc telemetry| HC
    HC -->|ProcessMetrics| AD
    AD -->|AnomalyReport| DE
    DM -.->|Dependency State| DE
    DE -->|DiagnosisRecord| RM
    RM -->|Pre-recovery snapshot| CP
    CP -->|Forensic Record| LOG
    RM -->|POSIX Signals / fork / exec| PROC
    RM -->|Verification Trigger| VE
    VE -.->|Post-recovery Check| PROC
    VE -->|VerificationResult| RM
    RM -->|IncidentCard| LOG
    HC -->|Live Telemetry| TUI
    LOG -->|Incident Summaries| TUI
```

---

## 2. Module Responsibilities & Call Hierarchy

| Subsystem / Class | Responsibility | Upstream Caller | Downstream Callees |
|---|---|---|---|
| **`ConfigManager`** | Parses `sentinel.conf`, validates allowlisted services, thresholds, and DAG relationships. | `main()` | All Subsystems |
| **`HealthCollector`** | Non-invasively reads `/proc/[pid]/stat` and `/proc/[pid]/status`. Calculates differential CPU% and RSS. | `main()` Event Loop | Linux Virtual Filesystem |
| **`AnomalyDetector`** | Maintains circular sliding windows of telemetry. Detects continuous rate-of-change $\frac{\Delta \text{Mem}}{\Delta t}$ and CPU spikes. | `main()` Event Loop | None (returns `AnomalyReport`) |
| **`DependencyManager`** | Evaluates Directed Acyclic Graph (DAG) states. Resolves cascade blocks (e.g., `App -> Backend -> DB`). | `main()` Event Loop | None (returns graph state) |
| **`DiagnosisEngine`** | Correlates anomalies with dependencies using deterministic rules to pinpoint root causes. | `main()` Event Loop | `AnomalyDetector`, `DependencyManager` |
| **`CheckpointManager`** | Creates immutable pre-recovery incident snapshots containing PID, args, telemetry, and timestamps. | `RecoveryManager` | Local Filesystem (`logs/`) |
| **`RecoveryManager`** | Enforces circuit breakers, coordinates tiered signal escalation (`SIGTERM` $\to$ `SIGKILL`), and respawns tasks. | `main()` Event Loop | `CheckpointManager`, `VerificationEngine`, Target |
| **`VerificationEngine`** | Validates post-restart runtime health across a 2-second stabilization window. | `RecoveryManager` | Target Process (`/proc/[new_pid]`) |
| **`Logger`** | Records human-readable audit trails and structured JSON incident cards. | All Subsystems | `logs/incidents.log`, `stdout` |

---

## 3. Subsystem Interaction Sequence Diagram

The following sequence represents a complete closed-loop lifecycle when a target process encounters a progressive memory exhaustion fault:

```mermaid
sequenceDiagram
    autonumber
    participant Target as fault_app (PID 1024)
    participant HC as HealthCollector
    participant AD as AnomalyDetector
    participant DE as DiagnosisEngine
    participant CP as CheckpointManager
    participant RM as RecoveryManager
    participant VE as VerificationEngine
    participant Log as Logger

    loop Every 1000ms Polling Interval
        HC->>Target: Poll /proc/1024/stat & status
        Target-->>HC: Telemetry (CPU, RSS, State)
        HC->>AD: Submit ProcessMetrics
        AD->>AD: Push to circular window; evaluate slope
    end

    Note over AD: 3 consecutive samples show positive slope (Mem growth > threshold)
    AD->>DE: Emit AnomalyReport (MEMORY_GROWTH)
    DE->>DE: Evaluate dependencies (DB & API healthy)
    DE->>RM: Issue DiagnosisRecord (OOM_RISK -> Action: GRACEFUL_RESTART)

    RM->>RM: Check Circuit Breaker (attempt 1 of 3 -> PERMITTED)
    RM->>CP: captureSnapshot(PID 1024, Metrics, Config)
    CP->>Log: Write Snapshot to Audit Trail

    RM->>Target: sendSignal(SIGTERM)
    Note over Target,RM: Await termination up to 3s grace timeout
    Target-->>RM: Process terminates (or SIGKILL if timeout expires)
    RM->>RM: reapZombie(waitpid)
    
    RM->>Target: fork() + execvp() [Respawn target]
    Note right of Target: New Process spawned (PID 2048)

    RM->>VE: verifyRecovery(PID 2048, BaselineSpec)
    loop Stabilization Window (2000ms)
        VE->>Target: Poll /proc/2048/status
        Target-->>VE: State == RUNNING, RSS == Normal
    end
    VE-->>RM: VerificationResult(SUCCESS, new_pid=2048)

    RM->>Log: logIncident(INC-0001, SUCCESS, Duration=2.1s)
    RM->>RM: Reset Circuit Breaker timer
```

---

## 4. State Machine Diagram

Each monitored service is governed by a formal finite state machine (FSM) to prevent illegal state transitions and infinite restart loops:

```mermaid
stateDiagram-v2
    [*] --> STOPPED: Registered in Config
    STOPPED --> HEALTHY: Process Spawned & Verified
    
    HEALTHY --> WARNING: 1-2 Threshold Breaches
    WARNING --> HEALTHY: Metrics Return to Baseline
    WARNING --> CRITICAL: K >= 3 Consecutive Samples Breached (or PID Terminated)
    
    CRITICAL --> CHECKPOINTING: Allowlist & Circuit Breaker Checked
    CRITICAL --> SAFE_MODE: Circuit Breaker Tripped (Retries > 3 in 60s)
    
    CHECKPOINTING --> RECOVERING: Snapshot Captured & Logged
    
    RECOVERING --> VERIFYING: Target Restarted (New PID Assigned)
    
    VERIFYING --> HEALTHY: New PID Stable & Probes Pass (Incident Closed)
    VERIFYING --> CRITICAL: New PID Crashes Immediately (Retry Count Incremented)
    
    SAFE_MODE --> [*]: Administrative Intervention Required
```

---

## 5. Class Diagram & Object-Oriented Structure

```mermaid
classDiagram
    class ProcessMetrics {
        +int pid
        +string service_name
        +double cpu_percent
        +long rss_kb
        +long vmsize_kb
        +char state
        +uint64_t timestamp_ms
    }

    class AnomalyReport {
        +string service_name
        +FaultType fault_type
        +Severity severity
        +int consecutive_samples
        +string message
        +uint64_t timestamp_ms
    }

    class IncidentSnapshot {
        +string incident_id
        +uint64_t timestamp_ms
        +int target_pid
        +string service_name
        +ProcessMetrics metrics
        +string config_version
        +string trigger_reason
    }

    class HealthCollector {
        -map~string, ProcessMetrics~ last_metrics
        +ProcessMetrics pollProcess(int pid, string name)
        +bool isProcessAlive(int pid)
    }

    class AnomalyDetector {
        -map~string, deque~ProcessMetrics~~ history_windows
        +optional~AnomalyReport~ evaluate(ProcessMetrics metrics)
        +void resetHistory(string service_name)
    }

    class DependencyManager {
        -map~string, vector~string~~ dependency_graph
        +bool areDependenciesHealthy(string service_name)
        +void registerDependency(string child, string parent)
    }

    class DiagnosisEngine {
        -DependencyManager* dep_mgr
        +DiagnosisRecord diagnose(AnomalyReport anomaly)
    }

    class CheckpointManager {
        +IncidentSnapshot capture(ProcessMetrics metrics, string reason)
        +bool saveToFile(IncidentSnapshot snapshot, string filepath)
    }

    class RecoveryManager {
        -map~string, int~ retry_counts
        -CheckpointManager* checkpoint_mgr
        -VerificationEngine* verification_engine
        +RecoveryResult executeRecovery(DiagnosisRecord diagnosis)
        +bool canRecover(string service_name)
    }

    class VerificationEngine {
        +VerificationResult verify(int new_pid, int timeout_ms)
    }

    class Logger {
        -string log_file_path
        +void logInfo(string msg)
        +void logIncident(IncidentCard card)
    }

    HealthCollector ..> ProcessMetrics : produces
    AnomalyDetector ..> ProcessMetrics : consumes
    AnomalyDetector ..> AnomalyReport : produces
    DiagnosisEngine ..> AnomalyReport : consumes
    DiagnosisEngine ..> DependencyManager : queries
    RecoveryManager ..> CheckpointManager : uses
    RecoveryManager ..> VerificationEngine : uses
    CheckpointManager ..> IncidentSnapshot : creates
    RecoveryManager ..> Logger : reports
```
