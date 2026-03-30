//
//  MainWindow.cpp
//  
//
//  Created by Aakash Jayaraj on 28/03/26.
//

#include "MainWindow.h"
#include "CpuSolverBackend.h"
#include "GpuMetalSolverBackend.h"
#include "ISolverBackend.h"
#include "ComputeConfig.h"

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
#include <QSpinBox>

#include <chrono>

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
      variableCombo_(new QComboBox(this)),
      perfLabel_(new QLabel("Backend: - | Grid: - | Steps: - | Time: -", this)),
      computeDeviceCombo_(new QComboBox(this)),
      cpuThreadsSpin_(new QSpinBox(this)) {

    auto* central = new QWidget(this);
    setCentralWidget(central);

    // Validators and default values
    nxEdit_->setValidator(new QIntValidator(10, 500, nxEdit_));
    nyEdit_->setValidator(new QIntValidator(10, 500, nyEdit_));
    reEdit_->setValidator(new QDoubleValidator(1.0, 1e6, 2, reEdit_));
    dtEdit_->setValidator(new QDoubleValidator(1e-6, 1.0, 6, dtEdit_));
    stepsEdit_->setValidator(new QIntValidator(1, 500000, stepsEdit_));

    nxEdit_->setText("65");
    nyEdit_->setText("65");
    reEdit_->setText("100");
    dtEdit_->setText("0.001");
    stepsEdit_->setText("2000");

    // Variable selector
    variableCombo_->addItem("Pressure");
    variableCombo_->addItem("Velocity magnitude");

    // Compute device selector
    computeDeviceCombo_->addItem("CPU (single core)");
    computeDeviceCombo_->addItem("CPU (multi-core)");
    computeDeviceCombo_->addItem("GPU (Metal, experimental)");

    cpuThreadsSpin_->setMinimum(1);
    cpuThreadsSpin_->setMaximum(16);   // adjust as you like
    cpuThreadsSpin_->setValue(4);

    // Left panel layout: compute + solver parameters
    auto* gridLayout = new QGridLayout();

    // Compute settings
    gridLayout->addWidget(new QLabel("Compute device:"), 0, 0);
    gridLayout->addWidget(computeDeviceCombo_,           0, 1);
    gridLayout->addWidget(new QLabel("CPU threads:"),    1, 0);
    gridLayout->addWidget(cpuThreadsSpin_,               1, 1);

    // CFD parameters
    gridLayout->addWidget(new QLabel("Nx:"),             2, 0);
    gridLayout->addWidget(nxEdit_,                       2, 1);
    gridLayout->addWidget(new QLabel("Ny:"),             3, 0);
    gridLayout->addWidget(nyEdit_,                       3, 1);
    gridLayout->addWidget(new QLabel("Re:"),             4, 0);
    gridLayout->addWidget(reEdit_,                       4, 1);
    gridLayout->addWidget(new QLabel("dt:"),             5, 0);
    gridLayout->addWidget(dtEdit_,                       5, 1);
    gridLayout->addWidget(new QLabel("Steps:"),          6, 0);
    gridLayout->addWidget(stepsEdit_,                    6, 1);

    gridLayout->addWidget(new QLabel("Variable:"),       7, 0);
    gridLayout->addWidget(variableCombo_,                7, 1);

    gridLayout->addWidget(runButton_,                    8, 0);
    gridLayout->addWidget(quitButton_,                   8, 1);

    // Right panel: image + status + progress + perf info
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    imageLabel_->setMinimumSize(300, 300);
    imageLabel_->setScaledContents(true);

    auto* rightLayout = new QVBoxLayout();
    rightLayout->addWidget(imageLabel_);
    rightLayout->addWidget(statusLabel_);
    rightLayout->addWidget(progressBar_);
    rightLayout->addWidget(perfLabel_);

    auto* mainLayout = new QHBoxLayout();
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(rightLayout);

    central->setLayout(mainLayout);

    // Connections
    connect(runButton_, &QPushButton::clicked,
            this, &MainWindow::onRunClicked);

    connect(quitButton_, &QPushButton::clicked,
            this, &MainWindow::onQuitClicked);

    setWindowTitle("MacCFD - Lid Driven Cavity");
    resize(900, 550);
}

void MainWindow::onRunClicked() {
    int nx = nxEdit_->text().toInt();
    int ny = nyEdit_->text().toInt();
    double Re = reEdit_->text().toDouble();
    double dt = dtEdit_->text().toDouble();
    int steps = stepsEdit_->text().toInt();

    ComputeOptions computeOpts = currentComputeOptions();

    QString backendName;
    if (computeOpts.backend == ComputeBackend::CpuSingle) {
        backendName = "CPU (single core)";
    } else if (computeOpts.backend == ComputeBackend::CpuMulti) {
        backendName = QString("CPU (multi-core, %1 threads)").arg(computeOpts.cpuThreads);
    } else {
        backendName = "GPU (Metal)";
    }

    statusLabel_->setText("Running on " + backendName + "...");
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    runButton_->setEnabled(false);
    qApp->processEvents();

    Grid grid(nx, ny, 1.0, 1.0);

    std::unique_ptr<ISolverBackend> backend;
    if (computeOpts.backend == ComputeBackend::CpuSingle ||
        computeOpts.backend == ComputeBackend::CpuMulti) {
        backend = std::make_unique<CpuSolverBackend>(grid, Re, dt);
        // In future, when you add a real multi-threaded backend,
        // you can pass computeOpts.cpuThreads into it here.
    } else {
        backend = std::make_unique<GpuMetalSolverBackend>(grid, Re, dt);
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int step = 0; step < steps; ++step) {
        backend->run(1);

        int percent = static_cast<int>(100.0 * (step + 1) / steps);
        progressBar_->setValue(percent);

        if (percent % 10 == 0 || step == steps - 1) {
            updateImageFromFields(backend->u(), backend->v(), backend->p());
            qApp->processEvents();
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    statusLabel_->setText("Done (" + backendName + ").");
    perfLabel_->setText(
        QString("Backend: %1 | Grid: %2×%3 | Steps: %4 | Time: %5 s")
            .arg(backendName)
            .arg(nx)
            .arg(ny)
            .arg(steps)
            .arg(elapsed, 0, 'f', 3)
    );

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

    // First pass: find min/max of selected variable
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

    // Second pass: map to colours
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

ComputeOptions MainWindow::currentComputeOptions() const {
    ComputeOptions opts;

    QString choice = computeDeviceCombo_->currentText();
    if (choice.startsWith("CPU (single")) {
        opts.backend = ComputeBackend::CpuSingle;
        opts.cpuThreads = 1;
    } else if (choice.startsWith("CPU (multi")) {
        opts.backend = ComputeBackend::CpuMulti;
        opts.cpuThreads = cpuThreadsSpin_->value();
    } else {
        opts.backend = ComputeBackend::GpuMetal;
        opts.cpuThreads = 1;
    }

    return opts;
}
