//
//  MainWindow.cpp
//  
//
//  Created by Aakash J on 28/03/26.
//

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

    // Variable selector
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

            // Update image every 10% to keep things responsive
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
        close();  // this will close the main window and exit the app
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

    // First pass: find min/max for the chosen variable
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
