#ifndef __NBODY_WORKER_H
#define __NBODY_WORKER_H

#include <vector>
#include <cal/cal.hpp>

class NBodyWorker
{
public:
    struct body_t {
        float x,y,z;
        float m;
    };

    struct velocity_t {
        float x,y,z;
    };

    struct options_t {
        int device;
        int num_bodies;
        int num_threads;
        int workitem_size;
        int tile_size;
        int read_count;
        int unroll_count;
        float dT;
        float eps2;
    };

protected:
    cal::Context      _context;
    cal::Device       _device;
    cal::Program      _program;
    cal::Kernel       _kernel;
    cal::CommandQueue _queue;
    cal::Image2D      _data[2];
    int               _active_buffer;
    int64_t           _exec_time;

public:
    options_t               opt;
    std::vector<body_t>     position;
    std::vector<velocity_t> velocity;

public:
    NBodyWorker();
    ~NBodyWorker();

    void init();

    void runKernel();
    void sendDataToGPU();
    void receiveDataFromGPU();

    void showFLOPS();
};

#endif
