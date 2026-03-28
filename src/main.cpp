//
//  main.cpp
//  
//
//  Created by Aakash J on 28/03/26.
//

#include <QApplication>
#include <QMetaType>
#include "MainWindow.h"
#include "Field.h"

int main(int argc, char* argv[]) {
    qRegisterMetaType<Field>("Field");
    
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}
