/*
 * Copyright (C) 2010, 2011 Artur Kornacki
 * 
 * This file is part of CAL++.
 *
 * CAL++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CAL++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CAL++.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Modification of matrix multiplication implementation presented by prunedtree ( http://forum.beyond3d.com/showthread.php?t=54842 ).
 * Algorithm computes A^T * B. Matrices are in row format.
 *
 * Changes to original version
 * - different data layout ( without matrices split )
 * - using pixelshader for tiled workitem layout
 * - workitem position computed directly in code ( in prunedtree's code it is precomputed )
 * - using sample offset ( proposed by nnsan http://galaxy.u-aizu.ac.jp/trac/note/wiki/MatrixMultiply )
 * - using mad ordering which tricks CAL compiler into generating efficient code ( corrected after looking at nnsan IL kernel,
 *   but this ordering is present in original prunedtree kernel )
 * - using sample id ( no impact on performance )
 * - using burst write
 *
 */

#ifdef _MSC_VER
  #pragma warning( disable : 4522 )
#endif

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cal/cal.hpp>
#include <cal/cal_il.hpp>

using namespace boost;
using namespace cal;
using namespace cal::il;

#define BX  8
#define BY  8
#define BX4 (BX/4)
#define BY4 (BY/4)

void kernel_matrixmul( input2d<float4>& A, input2d<float4>& B, global<float4>& C, 
                       const named_variable<float1>& xsize, const named_variable<float1>& ysize )
{
    float4  R[BY][BX4],ta[2][BY4],tb[2][BX4],p;
    int     i,j;

    assert( BX==8 && BY==8 );

    p.xy() = 2*ps::get_global_id<float2>();
    p.zw() = float2(-2,0);

    for(i=0;i<BY;i++) {
        for(j=0;j<BX4;j++) R[i][j]=0;
    }

    il_whileloop {
        p += float4(0,0,2,0);
        il_breakc( p.z()>=ysize );

        ta[0][0] = A( p.y()  , p.z() );
        ta[0][1] = A( p.y()+1, p.z() );
        tb[0][0] = B( p.x()  , p.z() );
        tb[0][1] = B( p.x()+1, p.z() );

        ta[1][0] = A( p.y()  , p.z()+1 );
        ta[1][1] = A( p.y()+1, p.z()+1 );
        tb[1][0] = B( p.x()  , p.z()+1 );
        tb[1][1] = B( p.x()+1, p.z()+1 );

        //
        // order of mads ( computations splited into 2 loops ) 
        // is important to trick CAL/IL compiler to generate efficient code ( saves 4 registers )
        //
        for(i=0;i<BY4;i++) {
            for(j=0;j<BX4;j++) {
                R[4*i+0][j] = mad( ta[0][i].xxxx(),tb[0][j], R[4*i+0][j] );
                R[4*i+1][j] = mad( ta[0][i].yyyy(),tb[0][j], R[4*i+1][j] );
                R[4*i+2][j] = mad( ta[0][i].zzzz(),tb[0][j], R[4*i+2][j] );
                R[4*i+3][j] = mad( ta[0][i].wwww(),tb[0][j], R[4*i+3][j] );
            }
        }

        il_breakc( xsize<0 ); // hack to reduce register usage

        for(i=0;i<BY4;i++) {
            for(j=0;j<BX4;j++) {
                R[4*i+0][j] = mad( ta[1][i].xxxx(),tb[1][j], R[4*i+0][j] );
                R[4*i+1][j] = mad( ta[1][i].yyyy(),tb[1][j], R[4*i+1][j] );
                R[4*i+2][j] = mad( ta[1][i].zzzz(),tb[1][j], R[4*i+2][j] );
                R[4*i+3][j] = mad( ta[1][i].wwww(),tb[1][j], R[4*i+3][j] );
            }
        }
    }
    il_endloop

    uint1 s,step;

    s    = cast_type<uint1>(p.y()*4*xsize + p.x());
    step = cast_type<uint1>(xsize);

    for(i=0;i<BY;i++) {
        for(j=0;j<BX4;j++) {
            C[s+j] = R[i][j];
        }
        s += step;
    }
}

std::string create_kernel_matrixmul()
{
    std::stringstream   code;

    code << "il_ps_2_0\n";
    code << "dcl_cb cb0[1]\n";

    Source::begin();

    input2d<float4>    A(0),B(1);
    global<float4>     C;

    kernel_matrixmul( A,B,C,named_variable<float1>("cb0[0].x"),named_variable<float1>("cb0[0].y") );

    Source::end();

    Source::emitHeader(code);
    Source::emitCode(code);

    return code.str();
}

//
//
//

#define ITER_COUNT      100
#define WIDTH           2048
#define HEIGHT          2048

Context         _context;
Program         _program;
Kernel          _kernel;
CommandQueue    _queue;

Image2D         _A,_B,_C;
int             _exec_time;

void fillMatrix( Image2D& m, float v )
{
    CALuint     pitch;
    float       *ptr,*line;
    int         width,height;

    width  = 4*m.getWidth();
    height = m.getHeight();

    line = (float*)_queue.mapMemObject(m,pitch);

    for(int i=0;i<height;i++) {
        ptr = line;
        for(int j=0;j<width;j++) *ptr++ = v;
        line += 4*pitch;
    }

    _queue.unmapMemObject(m);
}

void printMatrix( Image2D& m )
{
    CALuint pitch;
    float   *line,*ptr;
    int     width,height;

    width  = 4*m.getWidth();
    height = m.getHeight();

    line = (float*)_queue.mapMemObject(m,pitch);

    for(int i=0;i<height;i++) {
        ptr = line;
        for(int j=0;j<width;j++) {
            std::cout << boost::format("%.2f,") % *ptr++;
        }
        std::cout << "\n";
        line += 4*pitch;
    }

    _queue.unmapMemObject(m);
}

int init()
{
   _context = Context(CAL_DEVICE_TYPE_GPU);

    std::vector<Device> devices = _context.getInfo<CAL_CONTEXT_DEVICES>();

    // create program
    std::string source = create_kernel_matrixmul();
    //std::cout << source; // Uncomment to emit IL code
    _program = Program( _context, source.c_str(), source.length() );
    _program.build(devices);
    //_program.disassemble(std::cout); // Uncomment to emit ISA code

    // create kernel
    _kernel = Kernel(_program,"main");
    _kernel.setArgBind(0,"g[]");
    _kernel.setArgBind(1,"i0");
    _kernel.setArgBind(2,"i1");
    _kernel.setArgBind(3,"cb0",0,4);
    _kernel.setArgBind(4,"cb0",4,4);

    // allocate buffers
    _A = Image2D( _context, WIDTH/4, HEIGHT, CAL_FORMAT_FLOAT_4, 0 );
    _B = Image2D( _context, WIDTH/4, HEIGHT, CAL_FORMAT_FLOAT_4, 0 );

    _C  = Image2D( _context, WIDTH/4, HEIGHT, CAL_FORMAT_FLOAT_4, CAL_RESALLOC_GLOBAL_BUFFER );    

    return devices.size();
}

void setup( int dev )
{
    std::vector<Device> devices = _context.getInfo<CAL_CONTEXT_DEVICES>();
    _queue = CommandQueue(_context,devices[dev]);

    fillMatrix(_A,2);
    fillMatrix(_B,4);
}

void run()
{
    NDRange     rect;
    Event       event;

    rect = NDRange(_C.getWidth()/2,_C.getHeight()/8);

    posix_time::ptime t1 = posix_time::microsec_clock::local_time();

    _kernel.setArg(0,_C);
    _kernel.setArg(1,_A);
    _kernel.setArg(2,_B);
    _kernel.setArg(3,(float)_C.getWidth());
    _kernel.setArg(4,(float)_C.getHeight());

    for(int i=0;i<ITER_COUNT;i++) {
        _queue.enqueueNDRangeKernel( _kernel, rect, &event );
        _queue.flush();
        _queue.waitForEvent(event);
    }

    posix_time::ptime t2 = posix_time::microsec_clock::local_time();

    _exec_time = posix_time::time_period(t1,t2).length().total_microseconds();
}

void show_result( int dev )
{
    double tms      = (double)_exec_time/1000.;
    uint64_t mflops = ((uint64_t)2*(uint64_t)ITER_COUNT*(uint64_t)WIDTH*(uint64_t)HEIGHT*(uint64_t)WIDTH)/(uint64_t)_exec_time;
    double gflops   = (double)mflops/1000.;

    //printMatrix(_C);

    std::cout << format("Device %i: execution time %.2f ms, achieved %.2f gflops\n") % dev % tms % gflops;
}

void release_gpu_resources()
{
    _queue   = CommandQueue();
    _kernel  = Kernel();
    _context = Context();
    _program = Program();
}

int main( int argc, char* argv[] )
{
    int dev_count;

    cal::Init();

    dev_count = init();

    for(int i=0;i<dev_count;i++) {
        setup(i);
        run();
        show_result(i);
    }

    release_gpu_resources();
    cal::Shutdown();

    return 0;
}
