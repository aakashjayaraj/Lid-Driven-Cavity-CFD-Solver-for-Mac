# MacCFD – 2D Lid-Driven Cavity CFD Solver with CPU/GPU Backends (Qt + Metal, macOS)

MacCFD is a **simple 2D incompressible Navier–Stokes solver** for the lid‑driven cavity problem, built in C++ with a Qt GUI and support for **multiple compute backends**:

- CPU (single core)
- CPU (multi-core – work in progress)
- GPU (Metal, experimental) on Apple Silicon (M‑series)

It lets you set up, run, and visualise cavity flows directly on macOS with an interactive UI, and compare CPU vs GPU performance.

---

## 1. Features

### Physics & Numerics

- 2D incompressible Navier–Stokes equations on a uniform Cartesian grid.
- Lid‑driven cavity flow in a unit square domain `[0,1] × [0,1]`.
- Explicit convection/diffusion update for velocity.
- Pressure–velocity coupling via a **projection method**:
  - Intermediate velocity predictor.
  - Pressure Poisson solve with Jacobi iterations.
  - Velocity correction using pressure gradients.
- Simple boundary conditions:
  - No‑slip walls on left/right/bottom.
  - Moving lid at top (`u = 1`, `v = 0`).
  - Approximate zero normal gradient for pressure at boundaries.

### Compute Backends

Selectable from the GUI:

- **CPU (single core)**  
  - Uses the original C++ solver:
    - Velocity predictor (u*, v*).
    - RHS assembly (divergence of velocity).
    - Pressure Poisson solve via Jacobi iterations on CPU.
    - Velocity correction.
- **CPU (multi-core)** (UI + plumbing only; parallel solver to be added)
  - Currently uses the same code as CPU (single core).
  - UI allows specifying the desired number of CPU threads (future OpenMP/threads).
- **GPU (Metal, experimental)**  
  - Hybrid CPU/GPU solver:
    - Velocity predictor and RHS assembly on CPU.
    - **Pressure Poisson Jacobi iterations** offloaded to Metal:
      - Metal kernel `jacobiPressure2D` updates pressure on the GPU.
      - Pressure boundary conditions reapplied on CPU between iterations.
    - Velocity correction on CPU.
  - Implemented using:
    - Metal Shading Language (`.metal` kernel).
    - Objective‑C++ wrapper that:
      - Loads `GpuKernels.metal` at runtime.
      - Compiles it into a `MTLLibrary`.
      - Dispatches compute kernels for each Jacobi iteration.

### GUI (Qt Widgets)

- Native macOS window titled **“MacCFD – Lid Driven Cavity”**.
- Input fields:
  - `Nx`, `Ny` – grid resolution in x and y.
  - `Re` – Reynolds number.
  - `dt` – time step.
  - `Steps` – number of simulation steps.
- Variable selection:
  - `Pressure`
  - `Velocity magnitude`
- Compute device selection:
  - `CPU (single core)`
  - `CPU (multi-core)`
  - `GPU (Metal, experimental)`
- Multi-core control:
  - `CPU threads` spinbox for CPU multi-core mode (used in future parallel backend).
- Controls:
  - **Run simulation** button.
  - **Quit** button with confirmation dialog.
- Visualisation:
  - 2D colour map of the selected variable (pressure / |u|), updated during the run.
  - Linear blue–gray colour mapping with dynamic min/max scaling per frame.
- Status and performance tracking:
  - Status label showing current state (e.g. `Running on CPU (single core)...`, `Done (GPU (Metal)).`).
  - Progress bar (0–100%) updated as steps are completed.
  - **Performance label** at the bottom, e.g.:  
    `Backend: GPU (Metal) | Grid: 128×128 | Steps: 200 | Time: 0.318 s`

---

## 2. Installation (macOS, Apple Silicon)

### 2.1 Prerequisites

1. **Xcode Command Line Tools** (for compilers):

```bash
xcode-select --install
```

2. **Homebrew** (if not already installed):

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Add Homebrew to your `PATH` (Apple Silicon):

```bash
echo >> ~/.zprofile
echo 'eval "$(/opt/homebrew/bin/brew shellenv zsh)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv zsh)"
```

3. **Qt, CMake, LLVM**:

```bash
brew install qt cmake llvm
```

> Note: Qt is used for the GUI, LLVM can later be used for OpenMP / advanced compilation if needed.

### 2.2 Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/Lid-Driven-Cavity-CFD-Solver-for-Mac.git
cd Lid-Driven-Cavity-CFD-Solver-for-Mac
```

(or your actual repo name/URL).

---

## 3. Project Structure

```text
CFD_GUI/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    MainWindow.h
    MainWindow.cpp
    Grid.h
    Grid.cpp
    Field.h
    Field.cpp
    Solver.h
    Solver.cpp
    ComputeConfig.h
    ISolverBackend.h
    CpuSolverBackend.h
    CpuSolverBackend.cpp
    GpuMetalSolverBackend.h
    GpuMetalSolverBackend.cpp
    GpuMetalRunner.h
    GpuMetalRunner.mm
    GpuKernels.metal
  build/             (generated; ignored via .gitignore)
```

- `Solver.*` – original CPU solver for the cavity (used by CPU backend).
- `CpuSolverBackend.*` – wraps the CPU solver in a common backend interface.
- `GpuMetalSolverBackend.*` – hybrid CPU/GPU backend using Metal for pressure.
- `GpuMetalRunner.*` – Objective‑C++ wrapper to compile and launch Metal kernels.
- `GpuKernels.metal` – Metal compute kernel(s) (currently Jacobi pressure iteration).
- `MainWindow.*` – Qt GUI front‑end.

---

## 4. Building the Application

From the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

CMake will:

- Configure the project for C++17.
- Find Qt6 (from Homebrew).
- Enable Objective‑C++ for Metal integration.
- Link against Qt and the Metal / Foundation frameworks.
- Copy `GpuKernels.metal` into the build directory for runtime compilation.

You should end up with an executable `CFD_GUI` in the `build/` directory.

---

## 5. Running MacCFD

From `build/`:

```bash
./CFD_GUI
```

### 5.1 Basic CPU run (single core)

1. Set:
   - `Compute device`: **CPU (single core)**
   - `Nx = 65`, `Ny = 65`
   - `Re = 100`
   - `dt = 0.001`
   - `Steps = 2000`
   - `Variable = Pressure` (or `Velocity magnitude`)
2. Click **Run simulation**.
3. Watch:
   - Progress bar move.
   - Status label change to `Running on CPU (single core)...` and then `Done (CPU (single core)).`
   - Field visualisation animate as the solution develops.
   - Perf label update, e.g.:  
     `Backend: CPU (single core) | Grid: 65×65 | Steps: 2000 | Time: 3.557 s`

### 5.2 CPU multi-core mode (placeholder backend)

1. Set:
   - `Compute device`: **CPU (multi-core)**
   - `CPU threads`: e.g. `4`  
   - Other parameters as above.
2. Run again.
3. Currently, the same CPU solver is used; the multi‑core backend is planned for later (via OpenMP or threads). The perf label still shows the chosen backend and threads:

`Backend: CPU (multi-core, 4 threads) | Grid: 65×65 | Steps: 2000 | Time: 9.055 s`

### 5.3 GPU mode (Metal, experimental)

1. Ensure `GpuKernels.metal` is present in the `build/` directory (CMake copies it post‑build).
2. Set:
   - `Compute device`: **GPU (Metal, experimental)**
   - `Nx`, `Ny` – start with `65 × 65`, then try larger (e.g. `128 × 128`).
   - `Steps` – number of CFD time steps (each step includes many Jacobi iterations internally).
3. Click **Run simulation**.
4. The GPU backend will:
   - Compute intermediate velocities and RHS on CPU.
   - Run **pressure Poisson Jacobi iterations on the GPU** via the Metal kernel `jacobiPressure2D`.
   - Reapply pressure BCs on CPU.
   - Correct velocities on CPU.
5. The GUI shows e.g.:

`Backend: GPU (Metal) | Grid: 128×128 | Steps: 200 | Time: 0.318 s`

and you can compare performance against the CPU backends.

---

## 6. Implementation Details

### 6.1 Common Backend Interface

`ISolverBackend` defines a simple interface:

```cpp
class ISolverBackend {
public:
    virtual ~ISolverBackend() = default;
    virtual void run(int nSteps) = 0;
    virtual const Field& u() const = 0;
    virtual const Field& v() const = 0;
    virtual const Field& p() const = 0;
};
```

Each backend (CPU, GPU) implements this interface:

- `CpuSolverBackend` – wraps the original `Solver` class.
- `GpuMetalSolverBackend` – hybrid CPU/GPU solver with Metal for pressure.

The GUI never talks to the solver directly; it only sees an `ISolverBackend` pointer.

### 6.2 CPU Solver

The original `Solver` class:

- Manages `Grid`, `Field` objects for `u`, `v`, `p`, `rhs`.
- Implements:
  - `computeIntermediateVelocity()`
  - `solvePressurePoisson()`
  - `correctVelocity()`
- Applies velocity and pressure BCs for lid‑driven cavity.
- `run(nSteps, progressCallback)` performs the projection method loop on CPU.

`CpuSolverBackend` simply constructs `Solver` and calls `run`.

### 6.3 GPU Solver (GpuMetalSolverBackend)

The GPU backend reuses the same mathematics, but splits the work:

- CPU:
  - `computeIntermediateVelocityCpu()` – predictor step for u, v.
  - `computeRhsCpu()` – builds `rhs_` from divergence of u, v.
  - `applyPressureBCsCpu()` – Neumann BC for pressure.
  - `correctVelocityCpu()` – subtracts pressure gradients from u, v.
  - `applyVelocityBCsCpu()` – lid‑driven cavity BC for velocity.
- GPU (Metal):
  - `jacobiPressureStep(grid, p_, rhs_, pTemp_)` – one Jacobi iteration on pressure:
    - Metal kernel `jacobiPressure2D` updates interior cells of `p`.
    - BCs are reapplied on CPU.

The per-step logic in `run`:

```cpp
void GpuMetalSolverBackend::run(int nSteps) {
    for (int step = 0; step < nSteps; ++step) {
        computeIntermediateVelocityCpu();
        computeRhsCpu();

        int jacobiIters = 40;
        for (int it = 0; it < jacobiIters; ++it) {
            runner_->jacobiPressureStep(grid_, p_, rhs_, pTemp_);
            p_ = pTemp_;
            applyPressureBCsCpu();
        }

        correctVelocityCpu();
    }
}
```

### 6.4 Metal Integration (GpuMetalRunner + GpuKernels.metal)

- `GpuMetalRunner`:
  - On construction:
    - Creates `MTLDevice` and `MTLCommandQueue`.
    - Loads `GpuKernels.metal` source.
    - Compiles it into an `MTLLibrary` with `newLibraryWithSource`.
    - Looks up the `jacobiPressure2D` kernel.
    - Builds `MTLComputePipelineState`.
  - On `jacobiPressureStep`:
    - Copies `pOld` and `rhs` into GPU buffers.
    - Dispatches kernel over `nx × ny` threads.
    - Reads `pNew` back into the `Field` on CPU.

- `GpuKernels.metal`:
  - Defines the Jacobi kernel used in the Poisson solve.

---

## 7. Changes Introduced in the GPU/CPU Experimental Version

Compared to the initial version, this branch includes:

- **Backend abstraction**:
  - `ComputeConfig` (`ComputeBackend`, `ComputeOptions`).
  - `ISolverBackend` interface.
  - `CpuSolverBackend` and `GpuMetalSolverBackend` implementations.
- **GPU integration**:
  - `GpuMetalRunner` (Objective‑C++ Metal wrapper).
  - `GpuKernels.metal` (Metal compute kernels).
  - `CMakeLists.txt` updated to:
    - Enable Objective‑C++ (`enable_language(OBJCXX)`).
    - Link Metal and Foundation frameworks.
    - Copy `.metal` sources into the build directory.
- **GUI enhancements**:
  - Compute device selection dropdown (CPU single/multi, GPU Metal).
  - CPU threads spinbox (for future multi‑core backend).
  - Status label updated to show which backend is running.
  - New performance label at the bottom showing:
    - Backend
    - Grid size
    - Steps
    - Elapsed time
- **Stabilisation and cleanup**:
  - Proper initialisation of all fields in GPU backend (`u`, `v`, `uStar`, `vStar`, `p`, `rhs`, `pTemp`).
  - Size checks and error handling in `GpuMetalRunner::jacobiPressureStep`.
  - `.gitignore` added to exclude `build/` and CMake/moc output from version control.

---

## 8. Future Work

Planned or natural next steps:

- **CPU**:
  - Implement true multi‑core backend using OpenMP or `std::thread`, respecting the `CPU threads` setting in the GUI.

- **GPU**:
  - Keep fields (`u`, `v`, `p`, `rhs`) resident on GPU across iterations and time steps to reduce host↔device copies.
  - Move `computeRhs` and `computeIntermediateVelocity` to Metal.
  - Add residual monitoring and convergence criteria to the GPU Poisson solver.

- **Physics & GUI**:
  - Support additional cases (channel flow, backward‑facing step, cylinder).
  - Case selection dropdown and simple case configuration files (JSON/TOML).
  - Export results to VTK for ParaView / post‑processing.
