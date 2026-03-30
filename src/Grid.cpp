//
//  Grid.cpp
//  
//
//  Created by Aakash Jayaraj on 28/03/26.
//

#include "Grid.h"

Grid::Grid(std::size_t nx, std::size_t ny, double Lx, double Ly)
    : nx_(nx), ny_(ny), Lx_(Lx), Ly_(Ly) {
    dx_ = Lx_ / static_cast<double>(nx_ - 1);
    dy_ = Ly_ / static_cast<double>(ny_ - 1);
}
