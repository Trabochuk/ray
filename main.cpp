/*
 * Copyright (c) 2008-2011 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <cstdio>
#include <iostream>
#include <time.h>
#include <thread>

#include <Ray.h>
#include <RenderSurface.h>
#include <Tracer.h>
#include <Scene.h>

RenderSurface *gRenderSurface;
Scene *gScene;
TraceMaster *gTraceMaster;

#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace po = boost::program_options;

int TracerThread()
{
//  std::cout << "TracerThread start" << std::endl;
    time_t start = time(NULL);

    Tracer *tracer = new Tracer(*gRenderSurface, *gScene, *gTraceMaster);
    tracer->Trace();

    start = time(NULL) - start;
    std::cout << "trace end, took " << start << " seconds" << std::endl;

    delete tracer;

//  std::cout << "TracerThread end" << std::endl;

    return 0;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    int xres, yres, cpus;
    std::string outfile;
    bool random;

    // deal with options
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "help message")
    ("xres", po::value<int>(&xres)->default_value(800), "x resolution")
    ("yres", po::value<int>(&yres)->default_value(600), "y resolution")
    ("cpus,c", po::value<int>(&cpus)->default_value(8), "number cpus")
    ("out,o", po::value<std::string>(&outfile)->default_value(std::string("out.ray")), "output file")
    ("random,r", po::value<bool>(&random)->implicit_value(false), "random trace order")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    printf("xres %d yres %d cpus %d\n", xres, yres, cpus);
    std::cout << "outfile " << outfile << std::endl;

    gRenderSurface = new RenderSurfaceFile(xres, yres);

    (dynamic_cast<RenderSurfaceFile *>(gRenderSurface))->OpenOutFile(outfile);

    /* create the scene */
    gScene = new Scene();

    /* create a tracemaster */
    if (random) {
        gTraceMaster = new TraceMasterRandom(*gRenderSurface);
    } else {
        gTraceMaster = new TraceMasterSimple(*gRenderSurface);
    }

    std::vector<std::thread *> threads;
    for (int i = 0; i < cpus; i++) {
        auto *t = new std::thread(TracerThread);
        threads.push_back(t);
    }

    // wait for the trace master to quit handing out work units
    gTraceMaster->WaitForDone();

    // wait for the worker threads to complete
    std::cout << "waiting for worker threads to shut down" << std::endl;
    while (!threads.empty()) {
        auto *t = threads.back();
        threads.pop_back();
        t->join();
        delete t;
    }

//  std::cout << "writing output file..." << std::flush;
//  gRenderSurface->WriteTGAFile("foo.tga");
//  std::cout << "done" << std::endl;

    std::cout << "shutting down" << std::endl;

    delete gTraceMaster;
    delete gScene;
    delete gRenderSurface;

    return 0;
}

