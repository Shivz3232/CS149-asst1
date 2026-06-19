#include <fstream>
#include <iomanip>

#include "CycleTimer.h"

extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

extern void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations,
    int output[]);

extern void writePPMImage(
    int* data,
    int width, int height,
    const char *filename,
    int maxIterations);

bool verifyResult (int *gold, int *result, int width, int height) {

    int i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (gold[i * width + j] != result[i * width + j]) {
                printf ("Mismatch : [%d][%d], Expected : %d, Actual : %d\n",
                            i, j, gold[i * width + j], result[i * width + j]);
                return 0;
            }
        }
    }

    return 1;
}

void benchmark(int maxThreads,
               float x0, float y0,
               float x1, float y1,
               int width, int height,
               int maxIterations) {

    int* output_serial = new int[width * height];
    int* output_thread = new int[width * height];

    // -------------------------
    // Serial baseline
    // -------------------------
    double minSerial = 1e30;

    for (int i = 0; i < 5; i++) {
        memset(output_serial, 0, width * height * sizeof(int));

        double start = CycleTimer::currentSeconds();

        mandelbrotSerial(x0, y0, x1, y1,
                         width, height,
                         0, height,
                         maxIterations,
                         output_serial);

        double end = CycleTimer::currentSeconds();

        minSerial = std::min(minSerial, end - start);
    }

    std::ofstream csv("benchmark.csv");
    csv << "threads,time_ms,speedup\n";

    printf("Serial: %.3f ms\n", minSerial * 1000.0);
    writePPMImage(output_serial, width, height, "mandelbrot-serial.ppm", maxIterations);

    // -------------------------
    // Thread counts
    // -------------------------
    for (int numThreads = 1; numThreads <= maxThreads; numThreads++) {

        double minThread = 1e30;

        for (int run = 0; run < 5; run++) {

            memset(output_thread, 0, width * height * sizeof(int));

            double start = CycleTimer::currentSeconds();

            mandelbrotThread(numThreads,
                             x0, y0, x1, y1,
                             width, height,
                             maxIterations,
                             output_thread);

            double end = CycleTimer::currentSeconds();

            minThread = std::min(minThread, end - start);
        }

        if (!verifyResult(output_serial,
                          output_thread,
                          width,
                          height)) {

            fprintf(stderr,
                    "Verification failed for %d threads\n",
                    numThreads);
            continue;
        }

        double speedup = minSerial / minThread;

        printf("%2d threads : %8.3f ms  (%6.2fx)\n",
               numThreads,
               minThread * 1000.0,
               speedup);
        if (numThreads == 2) {
            writePPMImage(output_thread, width, height, "mandelbrot-thread.ppm", maxIterations);
        }

        csv << numThreads << ","
            << std::fixed << std::setprecision(3)
            << minThread * 1000.0 << ","
            << speedup << "\n";
    }

    csv.close();

    delete[] output_serial;
    delete[] output_thread;
}
