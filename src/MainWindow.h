//
//  MainWindow.h
//
//  Created by Aakash Jayaraj on 28/03/26.
//

#pragma once

#include <QMainWindow>
#include <QImage>
#include "Grid.h"
#include "Solver.h"
#include "ComputeConfig.h"

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QComboBox;
class QSpinBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onRunClicked();
    void onQuitClicked();

private:
    // CFD parameter inputs
    QLineEdit* nxEdit_;
    QLineEdit* nyEdit_;
    QLineEdit* reEdit_;
    QLineEdit* dtEdit_;
    QLineEdit* stepsEdit_;

    // Controls
    QPushButton* runButton_;
    QPushButton* quitButton_;
    QProgressBar* progressBar_;

    // Display widgets
    QLabel* statusLabel_;
    QLabel* imageLabel_;
    QLabel* perfLabel_;       // performance info (backend, grid, steps, time)
    QComboBox* variableCombo_;

    // Compute device selection
    QComboBox* computeDeviceCombo_;  // CPU/GPU selector
    QSpinBox*  cpuThreadsSpin_;      // number of CPU threads (for future multi-core backend)

    void updateImageFromFields(const Field& u, const Field& v, const Field& p);
    ComputeOptions currentComputeOptions() const;
};
