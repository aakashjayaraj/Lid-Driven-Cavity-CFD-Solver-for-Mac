//
//  GpuKernels.metal
//  
//
//  Created by Aakash Jayaraj on 30/03/26.
//

#include <metal_stdlib>
using namespace metal;

// One Jacobi iteration for the pressure Poisson equation
kernel void jacobiPressure2D(
    device const float* pOld   [[buffer(0)]],
    device const float* rhs    [[buffer(1)]],
    device       float* pNew   [[buffer(2)]],
    constant uint&      nx     [[buffer(3)]],
    constant uint&      ny     [[buffer(4)]],
    constant float&     dx     [[buffer(5)]],
    constant float&     dy     [[buffer(6)]],
    uint2               gid    [[thread_position_in_grid]])
{
    uint i = gid.x;
    uint j = gid.y;

    if (i == 0 || j == 0 || i >= nx - 1 || j >= ny - 1) {
        return;
    }

    uint idx   = j * nx + i;
    uint idx_l = idx - 1;
    uint idx_r = idx + 1;
    uint idx_b = idx - nx;
    uint idx_t = idx + nx;

    float dx2 = dx * dx;
    float dy2 = dy * dy;

    float term = (pOld[idx_r] + pOld[idx_l]) * dy2
               + (pOld[idx_t] + pOld[idx_b]) * dx2;

    float denom = 2.0f * (dx2 + dy2);

    pNew[idx] = (term - rhs[idx] * dx2 * dy2) / denom;
}
