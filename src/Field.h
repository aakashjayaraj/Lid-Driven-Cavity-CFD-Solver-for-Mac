//
//  Field.h
//  
//
//  Created by Aakash J on 28/03/26.
//

#pragma once
#include <vector>
#include <cstddef>
#include <QMetaType>

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

Q_DECLARE_METATYPE(Field)
