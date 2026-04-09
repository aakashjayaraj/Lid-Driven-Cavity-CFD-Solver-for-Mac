# MacCFD – 2D Lid-Driven Cavity CFD Solver with CPU/GPU Backends (Qt + Metal + OpenMP, macOS)

MacCFD is a **simple 2D incompressible Navier–Stokes solver** for the lid‑driven cavity problem, built in C++ with a Qt GUI and multiple compute backends:

- CPU (single core)
- CPU (multi‑core, via OpenMP)
- GPU (Metal, full time‑step on Apple Silicon)

It runs natively on macOS and lets you choose mesh size, Reynolds number, time step, number of steps, compute device, and variable to visualise (pressure or velocity magnitude). A built‑in performance panel reports backend, grid, steps, and wall‑clock time for each run.

---

## 1. Features

### 1.1 Physics & Numerics

- 2D incompressible Navier–Stokes equations on a uniform Cartesian grid.
- Lid‑driven cavity flow in a unit square domain `[0, 1] × [0, 1]`.
- Projection (fractional‑step) method:
  - Explicit predictor for intermediate velocity (`u*`, `v*`).
  - Pressure Poisson solve with Jacobi iterations.
  - Velocity correction using pressure gradients.
- Boundary conditions:
  - No‑slip stationary walls on left, right, and bottom.
  - Moving top lid (`u = 1`, `v = 0`).
  - Approximate zero normal pressure gradient at boundaries.
- Double precision on CPU; float precision on GPU (documented).

### 1.2 Compute Backends

All backends implement a common `ISolverBackend` interface so they can be swapped at runtime from the UI.

- **CPU (single core)**
  - Original C++ solver:
    - `computeIntermediateVelocity`
    - `solvePressurePoisson`
    - `correctVelocity`
  - Runs purely on a single thread, useful as a correctness and performance baseline.

- **CPU (multi‑core, OpenMP)**
  - Same solver and numerics as single‑core, but with:
    - `#pragma omp parallel for` on all heavy loops:
      - Predictor (u*, v*)
      - RHS assembly for Poisson
      - Jacobi updates in the pressure solve
      - Velocity correction
    - A dedicated backend (`CpuMultiSolverBackend`) that:
      - Stores its own `Grid` and `Solver`.
      - Calls `omp_set_num_threads(cpuThreads)` before each run.
  - Number of threads is controlled from the GUI (“CPU threads” spinbox).
  - Provides a clean, reproducible CPU multi‑core baseline for CFD.

- **GPU (Metal, full time‑step)**
  - Full Navier–Stokes time‑step runs on the Apple Silicon GPU via Metal:
    - Momentum predictor (u*, v*).
    - Boundary conditions (velocity and pressure).
    - RHS assembly for the pressure Poisson equation.
    - Jacobi pressure iterations.
    - Velocity correction.
  - Implemented via:
    - `GpuKernels.metal`:
      - `momentumU` / `momentumV` – compute `u*`, `v*`.
      - `applyVelocityBC` – apply lid/wall velocity boundary conditions.
      - `computeRHS` – build RHS for the Poisson equation.
      - `jacobiPressure2D` – Jacobi iteration for pressure.
      - `applyPressureBC` – Neumann‑like pressure boundary conditions.
      - `correctVelocity` – subtract pressure gradient from velocity.
    - `GpuMetalRunner` (Objective‑C++ wrapper):
      - Creates and owns persistent `MTLBuffer`s for:
        - `u`, `v`, `uStar`, `vStar`, `p`, `pNew`, `rhs`.
      - Encodes the **entire fractional step** into a single `MTLCommandBuffer` per time‑step:
        - Momentum kernels → BC kernels → RHS → multiple Jacobi iterations + BCs → correction → BCs.
      - Submits the command buffer once per CFD step and waits for completion.
      - Performs float↔double conversions on upload/download (shared memory buffers).
    - `GpuMetalSolverBackend`:
      - Owns CPU‑side `Field` objects (`u`, `v`, `p`).
      - Uses `GpuMetalRunner` to advance the solution `nSteps`.
      - Downloads GPU fields at the end of each run for visualisation.

This GPU path is now a **true GPU backend**: the entire time integration runs on the GPU, with the CPU used only for orchestration and GUI updates.

---

## 2. GUI and User Experience

The Qt GUI (`MainWindow`) is fully non‑blocking and structured for a “tool‑like” feel.

### 2.1 Controls

- **Compute device:**
  - `CPU (single core)`
  - `CPU (multi-core)`
  - `GPU (Metal, experimental)`

- **CPU threads:**
  - Integer spinner (e.g. 1–8), used when `CPU (multi-core)` is selected.

- **Simulation parameters:**
  - `Nx`, `Ny` – grid resolution in x and y.
  - `Re` – Reynolds number.
  - `dt` – time step.
  - `Steps` – number of simulation steps.

- **Variable for visualisation:**
  - `Pressure`
  - `Velocity magnitude`

- **Buttons:**
  - `Run simulation` – starts or requests stop for a simulation.
  - `Quit` – closes the application with confirmation.

### 2.2 Visualization & Performance Panel

- **Field display:**
  - Renders a 2D colour map of the chosen variable.
  - Currently uses a simple blue–gray mapping; can be extended to a full colormap (jet/viridis).

- **Status label:**
  - Shows backend and run state, e.g.:
    - `Running on CPU (multi-core, 4 threads)...`
    - `Done (GPU (Metal)).`

- **Progress bar:**
  - Shows step‑wise progress from 0% to 100%.

- **Performance label (bottom):**
  - Logs performance for each run, e.g.:
  -  
    `Backend: GPU (Metal) | Grid: 128×128 | Steps: 200 | Time: 0.318 s`

This makes it easy to compare CPU single‑core, CPU multi‑core, and GPU runs for the same case.

---

## 3. Architecture

### 3.1 Backend Interface

`ISolverBackend` defines a common interface:

- `void run(int nSteps)`
- `const Field& u() const`
- `const Field& v() const`
- `const Field& p() const`

Concrete backends:

- `CpuSolverBackend` – single‑core solver.
- `CpuMultiSolverBackend` – multi‑core solver using OpenMP.
- `GpuMetalSolverBackend` – GPU solver via Metal.

### 3.2 SolverWorker and QThread (non‑blocking UI)

The simulation runs on a background `QThread` via `SolverWorker`:

- `SolverWorker`:
  - Owns a `std::unique_ptr<ISolverBackend>` (CPU or GPU).
  - Runs `backend->run(1)` inside a loop for the requested number of steps.
  - Maintains:
    - `std::atomic<bool> stopRequested_` – for clean user‑requested stopping.
  - Signals:
    - `progress(int step, int total)` – step progress.
    - `fieldsUpdated(const Field& u, const Field& v, const Field& p)` – periodic field updates.
    - `finished()` – completed run.
    - `errorOccurred(const QString& message)` – error propagation from backends.

- `MainWindow`:
  - Creates a `QThread` and moves `SolverWorker` to it.
  - Connects worker signals to:
    - Progress bar.
    - Status and performance labels.
    - Image update.
    - Error dialog on failure.
  - `Run simulation` button:
    - Starts a new run if idle.
    - Sends `requestStop()` to the worker if already running (e.g. to stop early).

This design ensures the GUI remains responsive during long runs, and multiple runs can be started/stopped without blocking the event loop.

---

## 4. Implementation Details

### 4.1 Persistent GPU Buffers

`GpuMetalRunner` now allocates all Metal buffers **once** in its constructor:

- `id<MTLBuffer> uBuf, vBuf, uStarBuf, vStarBuf, pBuf, pNewBuf, rhsBuf;`

Buffers are created in `MTLResourceStorageModeShared`:

- CPU code writes directly into the same DRAM pages the GPU reads from.
- Upload and download are just float↔double loops; there is no extra `memcpy` of full arrays.

This replaces the previous approach that called `newBufferWithBytes` inside each Jacobi iteration, which would have required hundreds of thousands of allocations for a realistic run.

### 4.2 Single Metal Command Buffer per Time-Step

For each CFD time-step, `GpuMetalRunner::runStep`:

1. Encodes:
   - Velocity predictor (u*, v*).
   - Velocity BCs.
   - RHS build.
   - `N` Jacobi iterations + pressure BCs.
   - Velocity correction.
   - Final BCs.

2. Commits the command buffer once and waits for completion.

This minimises CPU↔GPU synchronisation overhead and lets the GPU execute the whole time-step as one coherent task.

### 4.3 CPU Multi-Core Design

- `CpuMultiSolverBackend` stores a `Grid` by value and a `Solver` using that grid.
- `run(nSteps)` sets the OpenMP thread count (`omp_set_num_threads`) and calls `solver.run`.
- OpenMP pragmas (`#pragma omp parallel for`) are only used in `Solver.cpp` on the hot loops, not in the GUI or GPU code.

On platforms with a stronger OpenMP toolchain (e.g. Linux + GCC/Clang), the same code can be built by swapping the CMake OpenMP section to a standard `find_package(OpenMP)` call.

---

## 5. Building and Running

### 5.1 Prerequisites (macOS, Apple Silicon)

- Xcode Command Line Tools:

  ```bash
  xcode-select --install
  ```

- Homebrew:

  ```bash
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  ```

  Then:

  ```bash
  echo >> ~/.zprofile
  echo 'eval "$(/opt/homebrew/bin/brew shellenv zsh)"' >> ~/.zprofile
  eval "$(/opt/homebrew/bin/brew shellenv zsh)"
  ```

- Qt + CMake + OpenMP runtime:

  ```bash
  brew install qt cmake libomp
  ```

### 5.2 Build (command line)

From the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
./CFD_GUI
```

### 5.3 Build & Run (Xcode via CMake)

You can also generate an Xcode project:

```bash
mkdir xcode-build
cd xcode-build
cmake -G Xcode ..
open CFD_GUI.xcodeproj
```

Then in Xcode:

- Scheme: `CFD_GUI`
- Destination: `My Mac`
- Run: ⌘R

---

## 6. Roadmap

This project is a **learning and demonstration tool** for building CFD software on Apple Silicon that looks and feels more like Star‑CCM+ or Fluent:

Short‑term planned improvements:

- Better colormap (jet/viridis).
- Residual monitoring and convergence criteria for the pressure solve.
- Stability checks (CFL, Reynolds‑dependent dt guidance) with GUI warnings.
- More canonical cases (channel flow, backward‑facing step) via pluggable boundary conditions.

Medium‑term:

- Reduce CPU↔GPU data transfers by keeping velocity and pressure on the GPU across multiple time‑steps.
- More of the solver logic in Metal (RHS, predictor, corrector) reusing the current full time‑step design.
- First turbulence or LES model for higher Reynolds numbers.

Long‑term:

- Case/mesh abstraction (beyond Cartesian grids).
- 2D unstructured mesh support via standard formats (VTK, etc.).
- Richer post‑processing (streamlines, vector plots) with GPU rendering.

---

## 7. Status

This branch represents **v0.x** of MacCFD:

- **Complete, working demo** of a 2D cavity solver with Qt GUI and multiple backends.
- **CPU single‑core, CPU multi‑core, and GPU Metal** backends all selectable from the UI.
- Designed as a stepping stone toward a more capable, Mac‑native CFD tool for research and learning.
