# ss-analytics

A head‑to‑head performance study of exact string searching on CPU vs. GPU using OpenCL and Vulkan (via Kompute). This repository contains:

- A CPU baseline implementation using C++23’s `std::string::find`
- An OpenCL 1.2 kernel (`searchAllLimited`) and host harness
- A Vulkan SPIR‑V compute shader wrapped in [Kompute](https://github.com/KomputeProject/kompute)
- See the Vulkan implementation on the [`9-vulkan-implementation` branch](https://github.com/wba6/ss-analytics/tree/9-vulkan-implimentation).
- A common benchmarking driver (`benchMarker.cpp`) that measures end‑to‑end latency and kernel execution times
- Scripts and data processing tools for generating synthetic corpora and exporting JSON traces

---

## Features

- **CPU Baseline**: Single-threaded `std::string::find` scan on AMD Ryzen 9 4900HS.
- **OpenCL Path**: Hand‑tuned 30‑line kernel, pinned‑memory zero‑copy support.
- **Vulkan Path**: SPIR‑V compute shader with Kompute for minimal dispatch overhead.
- **Cross‑Platform**: Tested on Apple M4 (macOS) and NVIDIA RTX 2060 Max‑Q (Linux).
- **Fine‑Grained Profiling**: Measures host-to-device transfer, queue latency, and pure device execution.

---

## Prerequisites

- **C++23 compiler** (e.g., `g++ 13.2` or `clang 17.0`)
- **CMake** ≥ 3.18
- **OpenCL 1.2** headers & ICD loader
- **Vulkan 1.1** SDK & loader
- **Kompute** library (v1.0+)
