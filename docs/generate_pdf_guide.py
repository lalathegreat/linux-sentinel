import os
import base64
import subprocess

PROJECT_DIR = "/Users/maa/Desktop/Project/linux-sentinel"
ASSETS_DIR = os.path.join(PROJECT_DIR, "docs/presentation_assets")
HTML_OUTPUT = os.path.join(PROJECT_DIR, "docs/screenshot_guide.html")
PDF_OUTPUT_LOCAL = os.path.join(PROJECT_DIR, "docs/Linux_Sentinel_Demo_Screenshots_Guide.pdf")
PDF_OUTPUT_ROOT = "/Users/maa/Desktop/Project/Linux_Sentinel_Demo_Screenshots_Guide.pdf"

def get_base64_img(filename):
    path = os.path.join(ASSETS_DIR, filename)
    if not os.path.exists(path):
        return ""
    with open(path, "rb") as f:
        data = f.read()
    return f"data:image/png;base64,{base64.b64encode(data).decode('utf-8')}"

img_mem_leak = get_base64_img("media_1790938275417.png")
img_circuit_breaker = get_base64_img("media_1790938518997.png")
img_shell_cmd = get_base64_img("media_1790938519003.png")
img_test_matrix = get_base64_img("media_1790938519007.png")
img_steady_state = get_base64_img("media_1790938519015.png")

html_template = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Linux Sentinel — Evaluator Demonstration & Screenshot Guide</title>
<style>
    @page {
        size: A4;
        margin: 14mm 14mm 16mm 14mm;
    }
    body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        color: #1a202c;
        line-height: 1.45;
        background-color: #ffffff;
        margin: 0;
        padding: 0;
        font-size: 10pt;
    }
    .page-break {
        page-break-before: always;
    }
    .header {
        border-bottom: 2px solid #3182ce;
        padding-bottom: 10px;
        margin-bottom: 16px;
    }
    .header-badge {
        display: inline-block;
        background-color: #ebf8ff;
        color: #2b6cb0;
        font-weight: 700;
        font-size: 8pt;
        text-transform: uppercase;
        letter-spacing: 0.8px;
        padding: 3px 8px;
        border-radius: 4px;
        border: 1px solid #bee3f8;
        margin-bottom: 6px;
    }
    h1 {
        color: #2b6cb0;
        font-size: 20pt;
        margin: 0 0 6px 0;
        font-weight: 800;
        letter-spacing: -0.5px;
    }
    .subtitle {
        color: #4a5568;
        font-size: 10.5pt;
        margin: 0;
        font-weight: 500;
    }
    h2 {
        color: #2d3748;
        font-size: 13pt;
        margin: 16px 0 8px 0;
        border-bottom: 1px solid #e2e8f0;
        padding-bottom: 4px;
        font-weight: 700;
    }
    h3 {
        color: #2b6cb0;
        font-size: 11pt;
        margin: 10px 0 5px 0;
        font-weight: 600;
    }
    .screenshot-container {
        text-align: center;
        margin: 10px 0;
        background: #0f172a;
        border-radius: 8px;
        padding: 6px;
        border: 1px solid #cbd5e1;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .screenshot-img {
        max-width: 100%;
        max-height: 250px;
        height: auto;
        border-radius: 4px;
        display: block;
        margin: 0 auto;
    }
    .script-box {
        background-color: #f7fafc;
        border-left: 4px solid #3182ce;
        padding: 8px 12px;
        margin: 8px 0;
        border-radius: 0 6px 6px 0;
        font-size: 9.5pt;
    }
    .script-box strong {
        color: #2b6cb0;
        display: block;
        margin-bottom: 3px;
        text-transform: uppercase;
        font-size: 8pt;
        letter-spacing: 0.5px;
    }
    .trap-box {
        background-color: #fffaf0;
        border-left: 4px solid #dd6b20;
        padding: 8px 12px;
        margin: 8px 0;
        border-radius: 0 6px 6px 0;
        font-size: 9pt;
    }
    .trap-box strong {
        color: #c05621;
        display: block;
        margin-bottom: 3px;
        text-transform: uppercase;
        font-size: 8pt;
        letter-spacing: 0.5px;
    }
    ul, ol {
        margin: 4px 0 8px 18px;
        padding: 0;
    }
    li {
        margin-bottom: 3px;
    }
    code {
        background-color: #edf2f7;
        color: #c53030;
        font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
        font-size: 8.5pt;
        padding: 1px 4px;
        border-radius: 3px;
        border: 1px solid #e2e8f0;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        margin: 8px 0;
        font-size: 8.5pt;
    }
    th, td {
        border: 1px solid #cbd5e0;
        padding: 5px 8px;
        text-align: left;
    }
    th {
        background-color: #edf2f7;
        color: #2d3748;
        font-weight: 700;
    }
    .badge-pass {
        display: inline-block;
        background-color: #c6f6d5;
        color: #22543d;
        padding: 2px 6px;
        border-radius: 3px;
        font-weight: bold;
        font-size: 7.5pt;
    }
</style>
</head>
<body>

<!-- PAGE 1: TITLE & EXECUTIVE SUMMARY -->
<div class="header">
    <div class="header-badge">Wipro Embedded Systems Capstone • Evaluator Defense Guide</div>
    <h1>Linux Sentinel — Screenshot Explanation & Presentation Playbook</h1>
    <p class="subtitle">A Student's Guide to Confidently Explaining Live Terminal Runs, Self-Healing, and Test Metrics to Your Technical Examiner</p>
</div>

<h2>1. Executive Summary: What You Are Demonstrating</h2>
<p>
    When presenting <strong>Linux Sentinel</strong> to your teacher or evaluator, your goal is to prove that this is a <strong>real, deterministic C++17 user-space systems orchestrator</strong>, not just a script or mockup. Traditional supervisors (such as <code>systemd</code>) only react <em>after</em> a process has died. Sentinel monitors the Linux kernel's <code>/proc</code> filesystem, detects degradation patterns (such as continuous memory leaks) <em>before</em> failure occurs, takes pre-recovery forensic snapshots, safely reaps and respawns the service, and verifies post-recovery stabilization.
</p>

<div class="script-box">
    <strong>The 30-Second Elevator Pitch (Say This to Your Teacher):</strong>
    "Sir/Ma'am, Linux Sentinel is a lightweight C++17 reliability orchestrator for embedded Linux devices. It observes process telemetry directly through the POSIX kernel /proc pseudo-filesystem. As demonstrated in these live runs, it intercepts progressive memory leaks, executes graceful restarts, enforces a 3-strike circuit breaker to prevent reboot loops, and passes all 12 test matrix scenarios with under 2.2 MB of RAM and 0.0% CPU overhead."
</div>

<h2>2. The Four Key Screens You Will Present</h2>
<table>
    <thead>
        <tr>
            <th>Screen / Demo</th>
            <th>Terminal Command</th>
            <th>Core Architectural Concept Demonstrated</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td><strong>Screen 1: Memory Leak Interception</strong></td>
            <td><code>./build/sentinel --spawn ./build/fault_app --leak</code></td>
            <td>Sliding window rate-of-change (+20 MB slope), graceful SIGTERM, PID respawn, 2s stabilization, Incident Card.</td>
        </tr>
        <tr>
            <td><strong>Screen 2: Circuit Breaker Safe-Mode</strong></td>
            <td><code>./build/sentinel --spawn ./build/fault_app --leak</code> (Cont.)</td>
            <td>Anti-flapping protection: Tripping after 3 restarts in 60s, locking target into SAFE_MODE to prevent reboot thrashing.</td>
        </tr>
        <tr>
            <td><strong>Screen 3: Baseline Normal Monitoring</strong></td>
            <td><code>./build/sentinel --spawn ./build/fault_app --normal</code></td>
            <td>Multi-sample noise filtering, non-invasive /proc polling, 0 false alarms over 40+ heartbeats.</td>
        </tr>
        <tr>
            <td><strong>Screen 4: Master 12-Test Matrix</strong></td>
            <td><code>./tests/run_tests.sh</code></td>
            <td>Unit tests + Full T1 to T12 test suite, 2.21 MB RSS, 0.0% CPU overhead, zero regressions.</td>
        </tr>
    </tbody>
</table>

<!-- PAGE 2: SCREENSHOT 1 BREAKDOWN -->
<div class="page-break"></div>
<h2>Screen 1: Live Memory Leak Interception & Self-Healing</h2>
<div class="screenshot-container">
    <img src="__IMG_MEM_LEAK__" class="screenshot-img" alt="Memory Leak Recovery Screenshot">
</div>

<h3>Line-by-Line Breakdown of What Is Happening:</h3>
<ol>
    <li><code>[INFO] [PID: 4065 ... RSS: 36 MB ... 41 MB ... 46 MB ... 51 MB]</code>: The target service <code>fault_app</code> allocates 5 MB every second, simulating a runaway memory leak.</li>
    <li><code>[WARN] ANOMALY: Monotonic memory growth detected: +20 MB over 5 sample intervals</code>: Sentinel calculates the rate of change. It does <em>not</em> wait for the Linux kernel OOM Killer to crash the OS.</li>
    <li><code>[WARN] DIAGNOSIS: Monotonic memory allocation slope without deallocation (Systemic Memory Leak)</code>: The Rule-Based Diagnosis Engine classifies the root cause and recommends a <code>GRACEFUL_RESTART</code>.</li>
    <li><code>[INFO] Forensic checkpoint captured: INC-0002</code>: Sentinel writes an immutable JSON snapshot of the process state to <code>logs/snapshots/</code> <em>before</em> touching the process.</li>
    <li><code>[fault_app] Received signal 15 (SIGTERM/SIGINT)... Clean exit completed</code>: Sentinel sends <code>SIGTERM</code> first. The child cleans up resources and exits gracefully. No forceful <code>SIGKILL</code> was needed.</li>
    <li><code>[SUCCESS] New process instance spawned with PID 4074</code>: Degraded PID 4065 was reaped to prevent a zombie process; a fresh instance was atomically launched.</li>
    <li><code>RECOVERY VERIFIED SUCCESSFUL in 2.428s</code>: Sentinel observed the new PID over a 2.0s stabilization window to ensure it remained alive and stable.</li>
    <li><strong>The Yellow Incident Card:</strong> Displays pre-action memory (51 MB), recovery action, new PID (4074), and exact latency (2.43s).</li>
</ol>

<div class="script-box">
    <strong>What to Say to Your Teacher:</strong>
    "In this screen, you can see Sentinel catching a progressive memory leak in real time. Instead of waiting for an out-of-memory crash, Sentinel's sliding window detected a +20 MB slope across 5 consecutive samples. It preserved a forensic checkpoint (INC-0002), sent a graceful SIGTERM signal, spawned a new instance with PID 4074, and verified stability for 2 seconds before closing the loop in 2.43 seconds."
</div>

<div class="trap-box">
    <strong>Examiner Question: "Why did Sentinel send SIGTERM instead of SIGKILL?"</strong><br>
    <strong>Your Answer:</strong> "Sending SIGKILL immediately can corrupt open file handles, leave uncommitted database transactions, or lock shared memory. Sentinel adheres to safe systems engineering: it sends SIGTERM with a 3000ms grace period. It only falls back to SIGKILL if the process becomes completely unresponsive."
</div>

<!-- PAGE 3: SCREENSHOT 2 BREAKDOWN -->
<div class="page-break"></div>
<h2>Screen 2: Anti-Flapping Circuit Breaker & Safe-Mode Lockout</h2>
<div class="screenshot-container">
    <img src="__IMG_CIRCUIT_BREAKER__" class="screenshot-img" alt="Circuit Breaker Screenshot">
</div>

<h3>Line-by-Line Breakdown of What Is Happening:</h3>
<ol>
    <li><code>INCIDENT CARD: INC-0003 ... Retry Count: 3</code>: The memory leak occurred repeatedly because the underlying application code contains a bug. Sentinel restarted it 3 times within 60 seconds.</li>
    <li><code>[ERROR] CIRCUIT BREAKER TRIPPED: Exceeded 3 restarts in 60s. Service placed into SAFE_MODE</code>: Sentinel realizes that auto-restarting is futile and harmful. It trips the circuit breaker.</li>
    <li><code>INCIDENT CARD: INC-0004 ... Recovery Action: ENTER_SAFE_MODE</code>: Sentinel records that the service has entered <code>SAFE_MODE</code> and halts auto-recovery.</li>
    <li><code>[ERROR] CRITICAL: Circuit breaker tripped! Halting auto-recovery for fault_app</code>: The supervisor stops restarting the process, terminates the degraded child cleanly, reaps the PID, and shuts down safely.</li>
</ol>

<div class="script-box">
    <strong>What to Say to Your Teacher:</strong>
    "Here you can see our Circuit Breaker protection mechanism. If an application has a fatal bug, simple supervisors like systemd will restart it indefinitely, causing a reboot storm that pegs the CPU and burns through embedded flash storage. Sentinel enforces a 3-strike policy: if a service restarts 3 times within 60 seconds, Sentinel trips the breaker, halts restarts, locks the service into SAFE_MODE, and logs an emergency Incident Card (INC-0004)."
</div>

<div class="trap-box">
    <strong>Examiner Question: "What is the danger of infinite restart loops in embedded Linux?"</strong><br>
    <strong>Your Answer:</strong> "In embedded devices, infinite restart loops cause thermal throttling, deplete device batteries, saturate logging partitions, and accelerate flash wear-out on eMMC or NAND storage. The Circuit Breaker protects the hardware from thrashing."
</div>

<!-- PAGE 4: SCREENSHOT 3 & SCREENSHOT 5 BREAKDOWN -->
<div class="page-break"></div>
<h2>Screen 3: Steady-State Baseline Monitoring (Zero False Alarms)</h2>
<div class="screenshot-container">
    <img src="__IMG_STEADY_STATE__" class="screenshot-img" alt="Steady State Baseline Screenshot">
</div>

<h3>Line-by-Line Breakdown:</h3>
<ul>
    <li><code>[PID: 4376 | fault_app | State: R | CPU: 0.0% | RSS: 16 MB | Threads: 1]</code>: Polling telemetry every second directly from <code>/proc</code>.</li>
    <li><code>Operating normally (Heartbeat 81 ... 121)</code>: Over 40+ consecutive intervals, memory stays flat at 16 MB, CPU is 0.0%, and zero anomalies are raised.</li>
</ul>

<div class="script-box">
    <strong>What to Say to Your Teacher:</strong>
    "This screen proves Sentinel's noise-immunity. An orchestrator that raises false alarms is worse than no orchestrator. By using a sliding window of 5 samples and requiring 3 consecutive breaches, normal background workload variations never trigger accidental restarts."
</div>

<h2>Crash Simulation (`kill -9`) & Shell Formatting</h2>
<div class="screenshot-container">
    <img src="__IMG_SHELL_CMD__" class="screenshot-img" alt="Shell command syntax error">
</div>
<div class="script-box">
    <strong>How to Explain This Screen:</strong>
    "In this terminal, <code>&lt;PID_FROM_TERMINAL_1&gt;</code> was typed literally, causing a shell parse error because <code>&lt;</code> and <code>&gt;</code> are redirection operators. In our live demo, we look at the PID logged by Sentinel (for example, PID <code>4376</code>) and run <code>kill -9 4376</code>. Sentinel immediately detects the zombie state (<code>State: Z</code>) and self-heals in under 1 second."
</div>

<!-- PAGE 5: SCREENSHOT 4 BREAKDOWN (TEST MATRIX) -->
<div class="page-break"></div>
<h2>Screen 4: Master 12-Test Matrix (T1–T12) & Benchmarks</h2>
<div class="screenshot-container">
    <img src="__IMG_TEST_MATRIX__" class="screenshot-img" alt="Automated Test Matrix Screenshot">
</div>

<h3>Summary of What Passed in Front of Your Eyes:</h3>
<table>
    <thead>
        <tr>
            <th>Test ID</th>
            <th>Scenario Name</th>
            <th>Requirement & Invariant Verified</th>
            <th>Result</th>
        </tr>
    </thead>
    <tbody>
        <tr><td><strong>T1</strong></td><td>Normal Baseline</td><td>Steady-state monitoring over 4s; zero false alarms.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T2</strong></td><td>Absent Target at Startup</td><td>Refuses to monitor non-existent PID; touches 0 unrelated processes.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T3</strong></td><td>Target Exits Mid-Run</td><td>Abrupt process exit detected; recovery evaluated.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T4</strong></td><td>Sustained CPU Load</td><td>CPU utilization sampled; persistence window verified.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T5</strong></td><td>Bounded Memory Growth</td><td>Monotonic slope detected; incident snapshot created.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T6</strong></td><td>Allowlisted Recovery</td><td>Allowlisted target relaunched; 2.0s stabilization passes.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T7 & T8</strong></td><td>Circuit Breaker & Cooldown</td><td>3-retry limit halts looping; cooldown window suppresses churn.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T9</strong></td><td>Malformed Config</td><td>Invalid configuration rejected with clear error; exits safely.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T10</strong></td><td>Disappears Mid-Read</td><td>Process dying mid-read handled cleanly; zero segmentation faults.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T11</strong></td><td>Unwritable Log Path</td><td>File write error caught; no false claim of disk persistence.</td><td><span class="badge-pass">PASS</span></td></tr>
        <tr><td><strong>T12</strong></td><td>Clean Build</td><td>Fresh CMake clean build succeeds with 0 warnings/errors.</td><td><span class="badge-pass">PASS</span></td></tr>
    </tbody>
</table>

<h3>Hardware Footprint & Benchmark Measurement:</h3>
<ul>
    <li><strong>Resident Memory (RSS):</strong> <code>2.21 MB</code> (Limit was 15.0 MB &rarr; <strong>85.3% below budget</strong>).</li>
    <li><strong>CPU Overhead:</strong> <code>0.0%</code> (Virtually zero CPU consumption during idle polling).</li>
    <li><strong>Outcome:</strong> <code>ALL 12 TEST SCENARIOS (T1 - T12) PASSED WITH ZERO REGRESSIONS</code>.</li>
</ul>

<div class="script-box">
    <strong>What to Say to Your Teacher:</strong>
    "Sir/Ma'am, this automated test suite validates all 12 scenarios from our project execution plan. Beyond unit tests, it tests edge cases like malformed config files, absent processes, unwritable logs, and circuit breaker tripping. The entire test matrix runs in 25 seconds, and Sentinel's runtime footprint is only 2.21 MB of RAM and 0.0% CPU overhead."
</div>

<!-- PAGE 6: QUICK DEFENSE CHEAT SHEET -->
<div class="page-break"></div>
<h2>Quick Reference: Evaluator Q&A Defense Cheat Sheet</h2>

<div class="trap-box">
    <strong>Q1: Why not just use systemd <code>Restart=always</code>?</strong><br>
    <strong>Answer:</strong> "Systemd only acts <em>after</em> a process has died. It cannot detect progressive memory leaks or CPU saturation while the process is technically still running. Furthermore, systemd lacks DAG dependency awareness—if an upstream database dies, systemd will repeatedly restart the downstream API gateway. Sentinel isolates the upstream root cause and intercepts degradation early."
</div>

<div class="trap-box">
    <strong>Q2: How does Sentinel calculate CPU percentage from <code>/proc</code>?</strong><br>
    <strong>Answer:</strong> "Process CPU ticks in Linux (<code>utime</code> and <code>stime</code>) are cumulative counters. Sentinel takes two consecutive samples, calculates the difference Delta ticks, divides by the system clock ticks per second (<code>sysconf(_SC_CLK_TCK)</code>), and divides by the elapsed wall-clock time Delta t to compute instantaneous CPU utilization."
</div>

<div class="trap-box">
    <strong>Q3: How do you prevent PID reuse vulnerabilities?</strong><br>
    <strong>Answer:</strong> "Linux PIDs wrap around and get recycled. Sentinel never relies on a raw PID alone. In our <code>ConfigManager</code> and <code>RecoveryManager</code>, we enforce a strict allowlist. We verify the executable path, startup arguments, and process start time before performing any signaling or recovery action."
</div>

<div class="trap-box">
    <strong>Q4: Why does Sentinel use C++17 without third-party libraries?</strong><br>
    <strong>Answer:</strong> "For embedded Linux systems, keeping binary footprint minimal and eliminating external runtime dependencies is critical. By relying exclusively on modern C++17 STL and native POSIX system calls, Sentinel compiles down to a single compact binary with no shared library version conflicts."
</div>

<div class="trap-box">
    <strong>Q5: What are the known limitations of Linux Sentinel?</strong><br>
    <strong>Answer:</strong> "1. It operates strictly in Linux user-space and does not replace kernel drivers or hardware watchdogs.<br>
    2. Its diagnosis is heuristic and rule-based rather than infallible hardware proof.<br>
    3. It is designed for single-node embedded reliability rather than distributed multi-node clusters."
</div>

<div style="margin-top: 30px; text-align: center; color: #718096; font-size: 8.5pt; border-top: 1px solid #e2e8f0; padding-top: 12px;">
    Linux Sentinel • Individual Systems Engineering Capstone • Repository: github.com/lalathegreat/linux-sentinel
</div>

</body>
</html>
"""

html_final = html_template.replace("__IMG_MEM_LEAK__", img_mem_leak) \
                          .replace("__IMG_CIRCUIT_BREAKER__", img_circuit_breaker) \
                          .replace("__IMG_STEADY_STATE__", img_steady_state) \
                          .replace("__IMG_SHELL_CMD__", img_shell_cmd) \
                          .replace("__IMG_TEST_MATRIX__", img_test_matrix)

with open(HTML_OUTPUT, "w", encoding="utf-8") as f:
    f.write(html_final)

print(f"Generated HTML at: {HTML_OUTPUT}")

# Convert HTML to PDF using Headless Google Chrome
chrome_path = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
cmd = [
    chrome_path,
    "--headless",
    "--disable-gpu",
    "--no-pdf-header-footer",
    f"--print-to-pdf={PDF_OUTPUT_LOCAL}",
    HTML_OUTPUT
]

result = subprocess.run(cmd, capture_output=True, text=True)
if result.returncode == 0:
    print(f"PDF successfully generated at: {PDF_OUTPUT_LOCAL}")
    subprocess.run(["cp", PDF_OUTPUT_LOCAL, PDF_OUTPUT_ROOT])
    print(f"Copied PDF to: {PDF_OUTPUT_ROOT}")
else:
    print(f"Error generating PDF: {result.stderr}")
