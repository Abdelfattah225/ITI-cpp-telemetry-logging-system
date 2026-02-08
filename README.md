# ITI-Telemetry-Logging-Sys

A scalable, multi-threaded logging system written in Modern C++17. Designed with best architectural and implementation practices in mind, utilizing the power of Modern C++ alongside industry-standard libraries and tools.

## Features

- ✅ Support for multiple local/remote data sources (File, Socket)
- ✅ Intuitive data formatting with threshold-based severity
- ✅ Support for multiple data storage strategies
- ✅ Extensible sink architecture using design patterns
- ✅ Thread-safe asynchronous logging operations
- ✅ High-performance ThreadPool for parallel sink writes
- ✅ RAII-compliant resource management
- ✅ SOME/IP integration with vSOME/IP
- ✅ Runtime Configuration using JSON
- ✅ Façade Pattern for simplified usage

## System Overview

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           DATA SOURCES (Phase 2 & 5)                         │
│   ┌───────────────┐   ┌───────────────┐   ┌───────────────┐                  │
│   │ CPU Telemetry │   │ RAM Telemetry │   │ GPU Telemetry │                  │
│   │   (File)      │   │   (File)      │   │   (File)      │                  │
│   └───────┬───────┘   └───────┬───────┘   └───────┬───────┘                  │
│           │                   │                   │                          │
│           └───────────────────┼───────────────────┘                          │
│                               │                                              │
│                               ▼                                              │
│                        FAÇADE (Phase 6)                                      │
│                  (TelemetryApp + Configuration)                              │
│                               │                                              │
│                               ▼                                              │
│   ┌──────────────────────────────────────────────────────────────────────┐   │
│   │                    LOG FORMATTER (Phase 3)                           │   │
│   │         Policy-based formatting with threshold severity              │   │
│   └──────────────────────────────┬───────────────────────────────────────┘   │
│                                  │                                           │
│                                  ▼                                           │
│   ┌──────────────────────────────────────────────────────────────────────┐   │
│   │                   ASYNC LOG MANAGER (Phase 4)                        │   │
│   │   ┌────────────────────────────────────────────────────────────┐     │   │
│   │   │              ThreadSafeRingBuffer<LogMessage>              │     │   │
│   │   └────────────────────────────┬───────────────────────────────┘     │   │
│   │                                │                                     │   │
│   │                                ▼                                     │   │
│   │                    ┌───────────────┼───────────────┐                 │   │
│   │                    ▼               ▼               ▼                 │   │
│   │              ┌──────────┐    ┌──────────┐    ┌──────────┐            │   │
│   │              │ Console  │    │   File   │    │  Socket  │            │   │
│   │              │   Sink   │    │   Sink   │    │   Sink   │            │   │
│   │              └──────────┘    └──────────┘    └──────────┘            │   │
│   └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

## Project Phases

| Phase | Description | Status |
|-------|-------------|--------|
| [Phase 1](docs/01-Core-Synchronous-Logging-Foundation/README.md) | Core Synchronous Logging Foundation | ✅ Complete |
| [Phase 2](docs/02-DataSources&SmartResourceManagement/README.md) | Data Sources & Smart Resource Management | ✅ Complete |
| [Phase 3](docs/03-Formatter&ThresholdLogic/README.md) | Formatter & Threshold Logic | ✅ Complete |
| [Phase 4](docs/04-AsyncLogging//README.md) | Asynchronous Logging with ThreadPool | ✅ Complete |
| [Phase 5](docs/05-Telemetry-Over-Network/README.md) | Telemetry Over Network (vSOME/IP) | ✅ Complete |
| [Phase 6](docs/06-System-Wrap-Up/README.md) | System Wrap Up (Façade & Configuration) | ✅ Complete |

## Topics Covered

### Modern C++17

| Topic | Phase | Description |
|-------|-------|-------------|
| Virtual Functions & Destructors | 1 | Dynamic dispatch for sink implementations |
| Operator Overloading | 1 | `operator<<` for LogMessage |
| Namespaces | 1 | Code organization |
| References | 2 | Parameter passing optimization |
| Move vs Copy Semantics | 2 | Efficient resource transfer |
| Rule of 3/5/0 | 2 | Special member functions |
| RAII | 2 | SafeFile, SafeSocket resource management |
| `enum class` | 3 | Type-safe enumerations |
| Templates | 3, 4 | LogFormatter, RingBuffer, ThreadSafeRingBuffer |
| `constexpr` & `if constexpr` | 3 | Compile-time computations |
| `std::optional` | 3, 4 | Nullable value types |
| `std::thread` | 4 | Multi-threading |
| `std::mutex` & `std::lock_guard` | 4 | Thread synchronization |
| `std::unique_lock` | 4 | Flexible locking |
| `std::condition_variable` | 4 | Thread coordination |
| `std::atomic` | 4 | Lock-free operations |
| `std::function` & Lambdas | 4 | Callables and task encapsulation |
| `std::future` & `std::packaged_task` | 4 | Asynchronous return values |
| Smart Pointers | All | `unique_ptr`, `shared_ptr` |
| `nlohmann/json` | 6 | Runtime configuration parsing |

### Design Patterns

| Category | Pattern | Phase | Usage |
|----------|---------|-------|-------|
| Behavioral | Strategy | 1 | Sink selection |
| Creational | Factory | 3 | LogSinkFactory |
| Creational | Builder | 3 | LogManagerBuilder |
| Concurrency | Producer-Consumer | 4 | Async logging flow |
| Concurrency | Thread Pool | 4 | Parallel sink writes |
| Structural | RAII | 2, 4 | Resource management |
| Creational | Singleton | 5 | vSOME/IP Application Instance |
| Structural | Adapter | 5 | vSOME/IP Client to ITelemetrySource |
| Structural | Façade | 6 | TelemetryApp hiding complexity |

### Design Idioms

| Idiom | Phase | Description |
|-------|-------|-------------|
| RAII | 2 | Resource Acquisition Is Initialization |
| Rule of Zero | 2, 3 | Delegate resource management |
| Rule of Five | 2 | Custom resource management |
| Policy-Based Design | 3 | Template policies for formatting |

## Libraries & Tools

| Library | Purpose | Status |
|---------|---------|--------|
| [GoogleTest](https://github.com/google/googletest) | Unit testing | ✅ Used |
| [magic_enum](https://github.com/Neargye/magic_enum) | Enum reflection | ✅ Used |
| [Bazel](https://bazel.build/) | Build system | ✅ Used |
| [vsomeip](https://github.com/COVESA/vsomeip) | SOME/IP communication | ✅ Used |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON Configuration | ✅ Used |
| [DLT](https://github.com/COVESA/dlt-daemon) | Diagnostic Log and Trace | 🔄 Planned |

## Project Structure

```
ITI_Cpp_LogLog_Project/
├── MODULE.bazel
├── WORKSPACE
├── README.md
│
├── inc/
│   ├── logging/           # Phase 1: Sinks & Core
│   ├── SmartDataHub/      # Phase 2: Sources & Phase 5: vSOME/IP
│   ├── Formatter/         # Phase 3: Formatting Policies
│   ├── AsyncLogging/      # Phase 4: ThreadPool & RingBuffer
│   └── Facade/            # Phase 6: TelemetryApp & Config
│
├── src/
│   ├── logging/
│   ├── SmartDataHub/
│   ├── Formatter/
│   ├── AsyncLogging/
│   └── Facade/
│
├── app/
│   ├── phase1/..phase4/   # Previous phase demos
│   ├── phase5/            # vSOME/IP Service & Client
│   └── phase6/            # Main Façade Application
│       ├── main.cpp
│       └── config.json
│
├── Utest/                 # Unit Tests
└── docs/                  # Detailed Documentation
    ├── 01..04-PhaseName/
    ├── 05-Telemetry-Over-Network/
    └── 06-System-Wrap-Up/
```

## Phase Highlights

### Phase 1: Core Synchronous Logging Foundation
- LogMessage structure with severity levels
- ILogSink interface with Strategy pattern
- ConsoleSinkImpl and FileSinkImpl
- LogManager for routing messages

### Phase 2: Data Sources & Smart Resource Management
- ITelemetrySource interface
- RAII wrappers: SafeFile, SafeSocket
- File and Socket telemetry sources

### Phase 3: Formatter & Threshold Logic
- Policy-based LogFormatter template
- Factory pattern for sink creation
- Builder pattern for LogManager

### Phase 4: Asynchronous Logging
- ThreadPool: Parallel task execution
- AsyncLogManager: Non-blocking logging

### Phase 5: Telemetry Over Network (vSOME/IP)
- vSOME/IP Service & Client implementation
- Adapter pattern for seamless integration
- Singleton pattern for middleware management

### Phase 6: System Wrap Up
- Façade pattern (`TelemetryApp`)
- JSON Runtime Configuration
- Simplified Application Lifecycle


## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| log() | O(1) | Non-blocking, push to buffer |
| RingBuffer push/pop | O(1) | Circular buffer operations |
| ThreadPool enqueue | O(1) | Amortized, queue insertion |
| Sink write | O(n) | Parallel via ThreadPool |
 Performance benchmarking suite

## Author

**Abdelfattah Moawed**
