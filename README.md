# CFD_GUI – 2D Lid-Driven Cavity CFD with Qt GUI (macOS)

This project is a simple 2D incompressible Navier–Stokes solver for the lid‑driven cavity problem, wrapped in a Qt‑based GUI that runs natively on macOS. It lets you choose mesh size, Reynolds number, time step, and number of steps, run the simulation interactively, and visualise either pressure or velocity magnitude, with a progress bar and a quit‑confirmation dialog.

---

## 1. Prerequisites (macOS)

### 1.1 Xcode command line tools

Install Apple’s command line tools (provides `clang` C++ compiler):

```bash
xcode-select --install
```

### 1.2 Homebrew

Install Homebrew (package manager) if you don’t already have it:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the on‑screen instructions. When it finishes, add Homebrew to your PATH (Apple Silicon path shown):

```bash
echo >> ~/.zprofile
echo 'eval "$(/opt/homebrew/bin/brew shellenv zsh)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv zsh)"
```

Verify:

```bash
brew --version
```

### 1.3 CMake and Qt

Install CMake and Qt via Homebrew:

```bash
brew install cmake qt
```

Confirm Qt is installed (paths may vary):

```bash
ls /opt/homebrew/opt/qt
```

---

## 2. Project Structure

Create the project directory and basic layout:

```bash
mkdir CFD_GUI
cd CFD_GUI
mkdir src
```

Final structure:

```
CFD_GUI/
  CMakeLists.txt
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
```

---

## 3. Build Configuration (CMake + Qt6)

Create `CMakeLists.txt` in the project root:

```cmake
cmake_minimum_required(VERSION 3.16)
project(CFD_GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Homebrew Qt path (Apple Silicon). Adjust if using Intel mac.
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/Grid.cpp
    src/Field.cpp
    src/Solver.cpp
)

set(HEADERS
    src/MainWindow.h
    src/Grid.h
    src/Field.h
    src/Solver.h
)

qt_wrap_cpp(MOC_SOURCES ${HEADERS})

add_executable(CFD_GUI
    ${SOURCES}
    ${MOC_SOURCES}
)

target_include_directories(CFD_GUI PRIVATE src)
target_link_libraries(CFD_GUI PRIVATE Qt6::Widgets)
```

This:

- Requires C++17.
- Points CMake to Homebrew’s Qt.
- Finds Qt6 Widgets.
- Generates Qt’s moc sources for headers.
- Builds a single executable `CFD_GUI`.

---

## 4. Numerical Core

### 4.1 Grid – 2D uniform mesh

`src/Grid.h`:

```cpp
#pragma once
#include <cstddef>

class Grid {
public:
    Grid(std::size_t nx, std::size_t ny, double Lx, double Ly);

    std::size_t nx() const { return nx_; }
    std::size_t ny() const { return ny_; }
    double dx() const { return dx_; }
    double dy() const { return dy_; }

private:
    std::size_t nx_, ny_;
    double Lx_, Ly_;
    double dx_, dy_;
};
```

`src/Grid.cpp`:

```cpp
#include "Grid.h"

Grid::Grid(std::size_t nx, std::size_t ny, double Lx, double Ly)
    : nx_(nx), ny_(ny), Lx_(Lx), Ly_(Ly) {
    dx_ = Lx_ / static_cast<double>(nx_ - 1);
    dy_ = Ly_ / static_cast<double>(ny_ - 1);
}
```

This defines a uniform grid on `[0, Lx] × [0, Ly]`.

---

### 4.2 Field – scalar values on the grid

`src/Field.h`:

```cpp
#pragma once
#include <vector>
#include <cstddef>

class Field {
public:
    Field() : nx_(0), ny_(0) {}
    Field(std::size_t nx, std::size_t ny)
        : nx_(nx), ny_(ny), data_(nx * ny, 0.0) {}

    void resize(std::size_t nx, std::size_t ny) {
        nx_ = nx;
        ny_ = ny;
        data_.assign(nx * ny, 0.0);
    }

    double& operator()(std::size_t i, std::size_t j) {
        return data_[j * nx_ + i];
    }

    double operator()(std::size_t i, std::size_t j) const {
        return data_[j * nx_ + i];
    }

    std::size_t nx() const { return nx_; }
    std::size_t ny() const { return ny_; }

    const std::vector<double>& data() const { return data_; }

private:
    std::size_t nx_, ny_;
    std::vector<double> data_;
};
```

`src/Field.cpp`:

```cpp
#include "Field.h"
// All methods are inline in the header.
```

Used for `u`, `v`, `p` as scalar fields on the grid.

---

### 4.3 Solver – 2D incompressible lid‑driven cavity

`src/Solver.h`:

```cpp
#pragma once
#include "Grid.h"
#include "Field.h"
#include <functional>

class Solver {
public:
    using ProgressCallback = std::function<void(int, int)>;

    Solver(const Grid& grid, double Re, double dt);

    void run(int nSteps, ProgressCallback progressCb = nullptr);

    const Field& u() const { return u_; }
    const Field& v() const { return v_; }
    const Field& p() const { return p_; }

private:
    const Grid& grid_;
    double Re_;
    double dt_;

    Field u_;
    Field v_;
    Field p_;
    Field uStar_;
    Field vStar_;
    Field rhs_;

    void applyBoundaryConditions();
    void computeIntermediateVelocity();
    void solvePressurePoisson(int iterations);
    void correctVelocity();
};
```

`src/Solver.cpp` (projection‑style scheme, compact but complete):

```cpp
#include "Solver.h"
#include <cmath>

Solver::Solver(const Grid& grid, double Re, double dt)
    : grid_(grid), Re_(Re), dt_(dt),
      u_(grid.nx(), grid.ny()),
      v_(grid.nx(), grid.ny()),
      p_(grid.nx(), grid.ny()),
      uStar_(grid.nx(), grid.ny()),
      vStar_(grid.nx(), grid.ny()),
      rhs_(grid.nx(), grid.ny()) {
    applyBoundaryConditions();
}

void Solver::applyBoundaryConditions() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();

    // No-slip stationary walls: left, right, bottom
    for (std::size_t j = 0; j < ny; ++j) {
        u_(0, j) = 0.0; v_(0, j) = 0.0;
        u_(nx - 1, j) = 0.0; v_(nx - 1, j) = 0.0;
    }
    for (std::size_t i = 0; i < nx; ++i) {
        u_(i, 0) = 0.0;
        v_(i, 0) = 0.0;
    }

    // Moving top lid: u = 1, v = 0
    for (std::size_t i = 0; i < nx; ++i) {
        u_(i, ny - 1) = 1.0;
        v_(i, ny - 1) = 0.0;
    }

    // Approximate zero normal pressure gradient
    for (std::size_t i = 0; i < nx; ++i) {
        p_(i, 0)      = p_(i, 1);
        p_(i, ny - 1) = p_(i, ny - 2);
    }
    for (std::size_t j = 0; j < ny; ++j) {
        p_(0, j)      = p_(1, j);
        p_(nx - 1, j) = p_(nx - 2, j);
    }
}

void Solver::computeIntermediateVelocity() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    // u*
    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double du2dx = (std::pow(u_(i + 1, j), 2) - std::pow(u_(i - 1, j), 2)) / (2.0 * dx);
            double duvdy = ((u_(i, j + 1) * v_(i, j + 1)) -
                            (u_(i, j - 1) * v_(i, j - 1))) / (2.0 * dy);

            double d2udx2 = (u_(i + 1, j) - 2.0 * u_(i, j) + u_(i - 1, j)) / dx2;
            double d2udy2 = (u_(i, j + 1) - 2.0 * u_(i, j) + u_(i, j - 1)) / dy2;

            double dpdx = (p_(i + 1, j) - p_(i - 1, j)) / (2.0 * dx);

            uStar_(i, j) = u_(i, j)
                + dt_ * ( -du2dx - duvdy - dpdx + (1.0 / Re_) * (d2udx2 + d2udy2) );
        }
    }

    // v*
    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double dv2dy = (std::pow(v_(i, j + 1), 2) - std::pow(v_(i, j - 1), 2)) / (2.0 * dy);
            double duvdx = ((u_(i + 1, j) * v_(i + 1, j)) -
                            (u_(i - 1, j) * v_(i - 1, j))) / (2.0 * dx);

            double d2vdx2 = (v_(i + 1, j) - 2.0 * v_(i, j) + v_(i - 1, j)) / dx2;
            double d2vdy2 = (v_(i, j + 1) - 2.0 * v_(i, j) + v_(i, j - 1)) / dy2;

            double dpdy = (p_(i, j + 1) - p_(i, j - 1)) / (2.0 * dy);

            vStar_(i, j) = v_(i, j)
                + dt_ * ( -dv2dy - duvdx - dpdy + (1.0 / Re_) * (d2vdx2 + d2vdy2) );
        }
    }

    u_ = uStar_;
    v_ = vStar_;
    applyBoundaryConditions();
}

void Solver::solvePressurePoisson(int iterations) {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();
    double dx2 = dx * dx;
    double dy2 = dy * dy;

    // RHS from divergence
    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double duxdx = (u_(i + 1, j) - u_(i - 1, j)) / (2.0 * dx);
            double dvydy = (v_(i, j + 1) - v_(i, j - 1)) / (2.0 * dy);
            rhs_(i, j) = (duxdx + dvydy) / dt_;
        }
    }

    Field pNew(nx, ny);

    for (int it = 0; it < iterations; ++it) {
        for (std::size_t j = 1; j < ny - 1; ++j) {
            for (std::size_t i = 1; i < nx - 1; ++i) {
                double term = ((p_(i + 1, j) + p_(i - 1, j)) * dy2 +
                               (p_(i, j + 1) + p_(i, j - 1)) * dx2);
                double denom = 2.0 * (dx2 + dy2);
                pNew(i, j) = (term - rhs_(i, j) * dx2 * dy2) / denom;
            }
        }

        for (std::size_t j = 1; j < ny - 1; ++j) {
            for (std::size_t i = 1; i < nx - 1; ++i) {
                p_(i, j) = pNew(i, j);
            }
        }

        applyBoundaryConditions();
    }
}

void Solver::correctVelocity() {
    std::size_t nx = grid_.nx();
    std::size_t ny = grid_.ny();
    double dx = grid_.dx();
    double dy = grid_.dy();

    for (std::size_t j = 1; j < ny - 1; ++j) {
        for (std::size_t i = 1; i < nx - 1; ++i) {
            double dpdx = (p_(i + 1, j) - p_(i - 1, j)) / (2.0 * dx);
            double dpdy = (p_(i, j + 1) - p_(i, j - 1)) / (2.0 * dy);

            u_(i, j) = u_(i, j) - dt_ * dpdx;
            v_(i, j) = v_(i, j) - dt_ * dpdy;
        }
    }

    applyBoundaryConditions();
}

void Solver::run(int nSteps, ProgressCallback progressCb) {
    for (int step = 0; step < nSteps; ++step) {
        computeIntermediateVelocity();
        solvePressurePoisson(40);
        correctVelocity();
        if (progressCb) {
            progressCb(step + 1, nSteps);
        }
    }
}
```

---

## 5. Qt GUI

### 5.1 Main window header

`src/MainWindow.h`:

```cpp
#pragma once

#include <QMainWindow>
#include <QImage>
#include "Grid.h"
#include "Solver.h"

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onRunClicked();
    void onQuitClicked();

private:
    QLineEdit* nxEdit_;
    QLineEdit* nyEdit_;
    QLineEdit* reEdit_;
    QLineEdit* dtEdit_;
    QLineEdit* stepsEdit_;
    QPushButton* runButton_;
    QPushButton* quitButton_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;
    QLabel* imageLabel_;
    QComboBox* variableCombo_;

    void updateImageFromFields(const Field& u, const Field& v, const Field& p);
};
```

---

### 5.2 Main window implementation

`src/MainWindow.cpp`:

```cpp
#include "MainWindow.h"

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QImage>
#include <QPixmap>
#include <QComboBox>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      nxEdit_(new QLineEdit(this)),
      nyEdit_(new QLineEdit(this)),
      reEdit_(new QLineEdit(this)),
      dtEdit_(new QLineEdit(this)),
      stepsEdit_(new QLineEdit(this)),
      runButton_(new QPushButton("Run simulation", this)),
      quitButton_(new QPushButton("Quit", this)),
      progressBar_(new QProgressBar(this)),
      statusLabel_(new QLabel("Ready", this)),
      imageLabel_(new QLabel(this)),
      variableCombo_(new QComboBox(this)) {

    auto* central = new QWidget(this);
    setCentralWidget(central);

    nxEdit_->setValidator(new QIntValidator(10, 500, nxEdit_));
    nyEdit_->setValidator(new QIntValidator(10, 500, nyEdit_));
    reEdit_->setValidator(new QDoubleValidator(1.0, 1e6, 2, reEdit_));
    dtEdit_->setValidator(new QDoubleValidator(1e-6, 1.0, 6, dtEdit_));
    stepsEdit_->setValidator(new QIntValidator(1, 50000, stepsEdit_));

    nxEdit_->setText("65");
    nyEdit_->setText("65");
    reEdit_->setText("100");
    dtEdit_->setText("0.001");
    stepsEdit_->setText("2000");

    variableCombo_->addItem("Pressure");
    variableCombo_->addItem("Velocity magnitude");

    auto* gridLayout = new QGridLayout();
    gridLayout->addWidget(new QLabel("Nx:"), 0, 0);
    gridLayout->addWidget(nxEdit_, 0, 1);
    gridLayout->addWidget(new QLabel("Ny:"), 1, 0);
    gridLayout->addWidget(nyEdit_, 1, 1);
    gridLayout->addWidget(new QLabel("Re:"), 2, 0);
    gridLayout->addWidget(reEdit_, 2, 1);
    gridLayout->addWidget(new QLabel("dt:"), 3, 0);
    gridLayout->addWidget(dtEdit_, 3, 1);
    gridLayout->addWidget(new QLabel("Steps:"), 4, 0);
    gridLayout->addWidget(stepsEdit_, 4, 1);

    gridLayout->addWidget(new QLabel("Variable:"), 5, 0);
    gridLayout->addWidget(variableCombo_, 5, 1);

    gridLayout->addWidget(runButton_, 6, 0);
    gridLayout->addWidget(quitButton_, 6, 1);

    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    imageLabel_->setMinimumSize(300, 300);
    imageLabel_->setScaledContents(true);

    auto* rightLayout = new QVBoxLayout();
    rightLayout->addWidget(imageLabel_);
    rightLayout->addWidget(statusLabel_);
    rightLayout->addWidget(progressBar_);

    auto* mainLayout = new QHBoxLayout();
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(rightLayout);

    central->setLayout(mainLayout);

    connect(runButton_, &QPushButton::clicked,
            this, &MainWindow::onRunClicked);

    connect(quitButton_, &QPushButton::clicked,
            this, &MainWindow::onQuitClicked);

    setWindowTitle("MacCFD - Lid Driven Cavity");
    resize(800, 500);
}

void MainWindow::onRunClicked() {
    int nx = nxEdit_->text().toInt();
    int ny = nyEdit_->text().toInt();
    double Re = reEdit_->text().toDouble();
    double dt = dtEdit_->text().toDouble();
    int steps = stepsEdit_->text().toInt();

    statusLabel_->setText("Running...");
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    runButton_->setEnabled(false);
    qApp->processEvents();

    Grid grid(nx, ny, 1.0, 1.0);
    Solver solver(grid, Re, dt);

    int lastPercent = 0;
    solver.run(steps, [&](int step, int total) {
        int percent = static_cast<int>(100.0 * step / total);
        if (percent != lastPercent) {
            lastPercent = percent;
            progressBar_->setValue(percent);

            // Update image every 10% to keep UI responsive
            if (percent % 10 == 0) {
                updateImageFromFields(solver.u(), solver.v(), solver.p());
                qApp->processEvents();
            }
        }
    });

    updateImageFromFields(solver.u(), solver.v(), solver.p());
    statusLabel_->setText("Done.");
    runButton_->setEnabled(true);
}

void MainWindow::onQuitClicked() {
    auto reply = QMessageBox::question(
        this,
        "Confirm Exit",
        "Are you sure you want to quit?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        close();
    }
}

void MainWindow::updateImageFromFields(const Field& u, const Field& v, const Field& p) {
    std::size_t nx = u.nx();
    std::size_t ny = u.ny();
    if (nx == 0 || ny == 0) return;

    QImage img(static_cast<int>(nx), static_cast<int>(ny), QImage::Format_RGB32);

    QString choice = variableCombo_->currentText();

    double minVal = 0.0, maxVal = 0.0;
    bool first = true;

    // First pass: find min/max
    for (std::size_t j = 0; j < ny; ++j) {
        for (std::size_t i = 0; i < nx; ++i) {
            double val;
            if (choice == "Pressure") {
                val = p(i, j);
            } else {
                double ui = u(i, j);
                double vi = v(i, j);
                val = std::sqrt(ui * ui + vi * vi);
            }

            if (first) {
                minVal = maxVal = val;
                first = false;
            } else {
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
        }
    }

    double range = maxVal - minVal;
    if (range == 0.0) range = 1.0;

    // Second pass: colour mapping
    for (std::size_t j = 0; j < ny; ++j) {
        for (std::size_t i = 0; i < nx; ++i) {
            double val;
            if (choice == "Pressure") {
                val = p(i, j);
            } else {
                double ui = u(i, j);
                double vi = v(i, j);
                val = std::sqrt(ui * ui + vi * vi);
            }

            double norm = (val - minVal) / range;
            int gray = static_cast<int>(norm * 255.0);
            if (gray < 0) gray = 0;
            if (gray > 255) gray = 255;

            img.setPixelColor(static_cast<int>(i),
                              static_cast<int>(ny - 1 - j),
                              QColor(gray, gray, 255 - gray));
        }
    }

    imageLabel_->setPixmap(QPixmap::fromImage(img));
}
```

---

### 5.3 Application entry point

`src/main.cpp`:

```cpp
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}
```

---

## 6. Build and Run

From the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build .
./CFD_GUI
```

In the GUI:

1. Set `Nx`, `Ny`, `Re`, `dt`, `Steps` (e.g. 65, 65, 100, 0.001, 2000).
2. Choose **Pressure** or **Velocity magnitude** from the Variable dropdown.
3. Click **Run simulation**.
4. Watch the progress bar and live updates of the selected field.
5. Use **Quit** → confirm to exit the application.

This README documents the full process: installing tools, creating the CFD core, wiring the Qt GUI, and building/running the final interactive CFD application on macOS.
