//
//  Grid.h
//  
//
//  Created by Aakash J on 28/03/26.
//

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
