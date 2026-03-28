//
//  MainWindow.h
//  
//
//  Created by Aakash J on 28/03/26.
//

// src/MainWindow.h
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
    QComboBox* variableCombo_;   // variable selector

    void updateImageFromFields(const Field& u, const Field& v, const Field& p);
};
