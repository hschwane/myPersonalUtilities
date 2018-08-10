/*
 * mpUtils
 * main.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail: hendrik.schwanekamp@gmx.net
 *
 * mpUtils = my personal Utillities
 * A utility library for my personal c++ projects
 *
 * Copyright 2016 Hendrik Schwanekamp
 *
 */

#include <mpUtils.h>
#include <cuda_gl_interop.h>
#include <thrust/random.h>

#include "particles/Particles.h"
#include "frontends/frontendInterface.h"
#include <Cuda/cudaUtils.h>
#include <crt/math_functions.hpp>
#include "particles/algorithms.h"

constexpr int BLOCK_SIZE = 256;
constexpr int PARTICLES = 1<<15;

int NUM_BLOCKS = (PARTICLES + BLOCK_SIZE - 1) / BLOCK_SIZE;

__global__ void generate2DNBSystem(Particles<DEV_POSM,DEV_VEL,DEV_ACC> pb)
{
    initializeEach(pb, [size=pb.size()](int i)
    {
        thrust::random::default_random_engine rng;
        rng.discard(i);
        thrust::random::uniform_real_distribution<float> dist(-1.0f,1.0f);

        Particle<POS,MASS,VEL> p;

        p.pos.x = dist(rng);
        p.pos.y = dist(rng);
        p.pos.z = 0.0f;
        p.mass = 1.0f/size;

        p.vel = cross(p.pos,{0.0f,0.0f, 0.75f});
        return p;
    });
}

__global__ void nbodyForces(Particles<DEV_POSM,DEV_VEL,DEV_ACC> particles, f1_t eps2, const int numTiles)
{
    SharedParticles<BLOCK_SIZE,SHARED_POSM> shared;

    const unsigned idx = blockIdx.x * blockDim.x + threadIdx.x;

    Particle<POS,MASS,VEL,ACC> pi = particles.loadParticle<POS,MASS,VEL>(idx);

    for (int tile = 0; tile < numTiles; tile++)
    {
        const auto p = particles.loadParticle<POS,MASS>(tile*blockDim.x+threadIdx.x);
        shared.storeParticle(threadIdx.x,p);

        __syncthreads();

        for(int j = 0; j<blockDim.x;j++)
        {
            auto pj = shared.loadParticle<POS,MASS>(j);
            f3_t r = pi.pos-pj.pos;
            f1_t distSqr = dot(r,r) + eps2;

            f1_t invDist = rsqrt(distSqr);
            f1_t invDistCube =  invDist * invDist * invDist;
            pi.acc -= r * pj.mass * invDistCube;

        }
        __syncthreads();
    }

    pi.acc -= pi.vel * 0.01;
    particles.storeParticle(idx,Particle<ACC>(pi));
}

__global__ void integrateLeapfrog(Particles<DEV_POSM,DEV_VEL,DEV_ACC> particles, f1_t dt, bool not_first_step)
{
    using pT=Particle<POS,VEL,ACC>;
    doForEach(particles, nvstd::function<pT(pT)>([&](Particle<POS,VEL,ACC> pi)
    {
        //   calculate velocity a_t
        pi.vel = pi.vel + pi.acc * (dt * 0.5f);

        // we could now change delta t here

        // calculate velocity a_t+1/2
        pi.vel = pi.vel + pi.acc * (dt * 0.5f) * not_first_step;

        // calculate position r_t+1
        pi.pos = pi.pos + pi.vel * dt;

        return pi;
    }));
}

int main()
{
    mpu::Log myLog( mpu::LogLvl::ALL, mpu::ConsoleSink());

    logINFO("pfSPH") << "Welcome to planetformSPH!";
    assert_cuda(cudaSetDevice(0));

    // set up frontend
    fnd::initializeFrontend();
    bool simShouldRun = true;
    fnd::setPauseHandler([&simShouldRun](bool pause){simShouldRun = !pause;});

    // generate some particles
    Particles<DEV_POSM,DEV_VEL,DEV_ACC> pb(PARTICLES);

    // register position and velocity buffer with cuda
#if defined(FRONTEND_OPENGL)
    pb.registerGLGraphicsResource<DEV_POSM>(fnd::getPositionBuffer(pb.size()));
    pb.registerGLGraphicsResource<DEV_VEL>(fnd::getVelocityBuffer(pb.size()));
    pb.mapGraphicsResource();
#endif

    generate2DNBSystem<<<NUM_BLOCKS,BLOCK_SIZE>>>(pb.createDeviceCopy());
    assert_cuda(cudaGetLastError());
    assert_cuda(cudaDeviceSynchronize());

    nbodyForces<<<NUM_BLOCKS,BLOCK_SIZE>>>(pb.createDeviceCopy(),0.0001f, PARTICLES/ BLOCK_SIZE);
    assert_cuda(cudaGetLastError());
    integrateLeapfrog<<<NUM_BLOCKS,BLOCK_SIZE>>>(pb.createDeviceCopy(),0.001f,false);
    assert_cuda(cudaGetLastError());

    pb.unmapGraphicsResource(); // used for frontend stuff
    mpu::DeltaTimer dt;
    while(fnd::handleFrontend(dt.getDeltaTime()))
    {
        if(simShouldRun)
        {
            pb.mapGraphicsResource(); // used for frontend stuff

            nbodyForces<<<NUM_BLOCKS,BLOCK_SIZE>>>(pb.createDeviceCopy(),0.0001f, PARTICLES/ BLOCK_SIZE);
            assert_cuda(cudaGetLastError());
            integrateLeapfrog<<<NUM_BLOCKS,BLOCK_SIZE>>>(pb.createDeviceCopy(),0.001f,true);
            assert_cuda(cudaGetLastError());

            pb.unmapGraphicsResource(); // used for frontend stuff
        }
    }

    return 0;
}