#include "OpenCL/OpenClParticleSimulator.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <OpenCL/opencl.h>

#include "OpenCL/opencl_context_info.hpp"

#include "common/FileReader.hpp"

#include "OpenCL/clVoxelCell.hpp"

#include "Parameters.h"

void Exit() {
    std::exit(1);
}

OpenClParticleSimulator::~OpenClParticleSimulator() {
    // TODO clean up allocated space on the GPU (clRelease[...] ?)
}

void OpenClParticleSimulator::createAndBuildKernel(cl_kernel &kernel_out, std::string kernel_name,
                                                   std::string kernel_file_name) {
    std::cout << "Creating kernel \"" << kernel_name << "\" from file kernels/" << kernel_file_name << ".\n\n";
    const auto kernel_str = FileReader::ReadFromFile("../kernels/" + kernel_file_name);

    const char *kernel_cstr = kernel_str.c_str();
    size_t kernel_str_size = std::strlen(kernel_str.c_str());
    std::cout << kernel_str << "\n" << "Kernel program length = " << kernel_str_size << "\n";

    cl_int error = CL_SUCCESS;

    cl_program program = clCreateProgramWithSource(context, 1, &kernel_cstr,
                                                   (const size_t *) &kernel_str_size, &error);

    CheckError(error);
    error = clBuildProgram(program, 1, &deviceIds[chosen_device_id - 1], NULL, NULL, NULL);
    if (error == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, deviceIds[chosen_device_id - 1], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, deviceIds[chosen_device_id - 1], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        std::cout << log << "\n";
    }

    CheckError(error);

    kernel_out = clCreateKernel(program, kernel_name.c_str(), &error);
    CheckError(error);
}

void OpenClParticleSimulator::setupSharedBuffers(const GLuint &vbo_positions, const GLuint &vbo_velocities) {
    cl_int error;

    // R/W buffers
    cl_positions = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, vbo_positions, &error);
    CheckError(error);
    error = clRetainMemObject(cl_positions);
    CheckError(error);
    cl_velocities = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, vbo_velocities, &error);
    CheckError(error);
    error = clRetainMemObject(cl_velocities);
    CheckError(error);

    // R buffers
    cl_positions_readonly = clCreateFromGLBuffer(context, CL_MEM_READ_ONLY, vbo_positions, &error);
    CheckError(error);
    error = clRetainMemObject(cl_positions_readonly);
    CheckError(error);
    cl_velocities_readonly = clCreateFromGLBuffer(context, CL_MEM_READ_ONLY, vbo_velocities, &error);
    CheckError(error);
    error = clRetainMemObject(cl_velocities_readonly);
    CheckError(error);

    // W buffers
    cl_positions_writeonly = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, vbo_positions, &error);
    CheckError(error);
    error = clRetainMemObject(cl_positions_writeonly);
    CheckError(error);
    cl_velocities_writeonly = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, vbo_velocities, &error);
    CheckError(error);
    error = clRetainMemObject(cl_velocities_writeonly);
    CheckError(error);
}

void OpenClParticleSimulator::allocateVoxelGridBuffer() {
    using namespace Parameters;
    cl_int error = CL_SUCCESS;

    const unsigned int grid_size_x = static_cast<unsigned int>(ceilf(get_volume_size_x() / kernelSize));
    const unsigned int grid_size_y = static_cast<unsigned int>(ceilf(get_volume_size_y() / kernelSize));;
    const unsigned int grid_size_z = static_cast<unsigned int>(ceilf(get_volume_size_z() / kernelSize));;

    const unsigned int grid_cell_count = grid_size_x * grid_size_y * grid_size_z;

    std::cout << "Grid size [x=" << grid_size_x << " y=" << grid_size_y << " z=" << grid_size_z <<
    "], total cells = " << grid_cell_count << "\n";

    std::vector<clVoxelCell> voxel_grid_cells(grid_cell_count);

    clVoxelCell &cell = voxel_grid_cells[0];
    clParticle &p = cell.particles[3];

    // Just to check that all voxel cell particles are initialized to zero
    std::cout << print_clParticle(p);

    cl_voxel_grid = clCreateBuffer(context,
                                   CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                   grid_cell_count * sizeof(clVoxelCell),
                                   (void *) voxel_grid_cells.data(),
                                   &error);
    CheckError(error);
}

void OpenClParticleSimulator::setupSimulation(const std::vector<glm::vec3> &particle_positions,
                                              const std::vector<glm::vec3> &particle_velocities,
                                              const GLuint &vbo_positions,
                                              const GLuint &vbo_velocities) {
    initOpenCL();
    std::cout << "\nOpenCL ready to use: context created.\n\n";

    n_particles = particle_positions.size();

    // Here we can use OpenCL functionality
    // cl_int error = CL_SUCCESS;

    setupSharedBuffers(vbo_positions, vbo_velocities);
    allocateVoxelGridBuffer();

    createAndBuildKernel(simple_integration, "taskParallelIntegrateVelocity", "update_particle_positions.cl");
}

void OpenClParticleSimulator::updateSimulation(float dt_seconds) {
    // Make sure all OpenGL commands will run before enqueueing OpenCL kernels
    glFlush();

    std::vector<cl_event> events;

    runSimpleIntegratePositionsKernel(dt_seconds, events);

//    runPopulateVoxelGridKernel(events);
//    runCalculateParticleDensitiesKernel(events);
//    runCalculateParticleForcesAndIntegrateStatesKernel(events);
//    runCopyParticlesToOpenGlBufferKernel(events);

    clWaitForEvents(events.size(), (const cl_event *) events.data());
}

void OpenClParticleSimulator::initOpenCL() {
    cl_uint platformIdCount = 0;
    clGetPlatformIDs(0, nullptr, &platformIdCount);


    if (platformIdCount == 0) {
        std::cerr << "No OpenCL platform found" << std::endl;
        Exit();
    } else {
        std::cout << "Found " << platformIdCount << " platform(s)" << std::endl;
    }

    // Resize vector to actual amount of platform ids and get the ids
    platformIds.resize(platformIdCount);
    clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

    for (cl_uint i = 0; i < platformIdCount; ++i) {
        std::cout << "\t (" << (i + 1) << ") : " << GetPlatformName(platformIds[i]) << std::endl;
    }

    cl_uint deviceIdCount = 0;
    clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, 0, nullptr,
                   &deviceIdCount);

    if (deviceIdCount == 0) {
        std::cerr << "No OpenCL devices found" << std::endl;
        Exit();
    } else {
        std::cout << "Found " << deviceIdCount << " device(s)" << std::endl;
    }

    deviceIds.resize(deviceIdCount);
    clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, deviceIdCount,
                   deviceIds.data(), nullptr);

    for (cl_uint i = 0; i < deviceIdCount; ++i) {
        std::cout << "\t (" << (i + 1) << ") : " << GetDeviceName(deviceIds[i]) << std::endl;
    }

    cl_int error = CL_SUCCESS;

#ifdef linux
#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
        0
    };
#elif defined _WIN32
#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) platformIds[0],
        0
    };
#elif defined TARGET_OS_MAC
#define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"

    CGLContextObj glContext = CGLGetCurrentContext();
    CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
    cl_context_properties properties[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties) shareGroup,
            0};

    gcl_gl_set_sharegroup(shareGroup);
#endif

    context = clCreateContext(properties, deviceIdCount,
                              deviceIds.data(), nullptr, nullptr, &error);
    CheckError(error);

    std::cout << "Context created" << std::endl;

    std::cout << "Choose a device id from the devices above: ";

    std::cin >> chosen_device_id;

    command_queue = clCreateCommandQueue(context, deviceIds[chosen_device_id - 1],
                                         0, &error);

    CheckError(error);

    // Check that context sharing is supported
    bool cgl_context_sharing_supported = false;

    size_t extensionSize;
    error = clGetDeviceInfo(deviceIds[chosen_device_id - 1], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize);

    CheckError(error);

    if (extensionSize > 0) {
        char *extensions = (char *) malloc(extensionSize);
        error = clGetDeviceInfo(deviceIds[chosen_device_id - 1], CL_DEVICE_EXTENSIONS, extensionSize, extensions,
                                &extensionSize);
        CheckError(error);

        std::string stdDevString(extensions);
        free(extensions);

        size_t szOldPos = 0;
        size_t szSpacePos = stdDevString.find(' ', szOldPos); // extensions string is space delimited
        while (szSpacePos != stdDevString.npos) {
            if (strcmp(GL_SHARING_EXTENSION, stdDevString.substr(szOldPos, szSpacePos - szOldPos).c_str()) == 0) {
                // Device supports context sharing with OpenGL
                cgl_context_sharing_supported = true;
                break;
            }
            do {
                szOldPos = szSpacePos + 1;
                szSpacePos = stdDevString.find(' ', szOldPos);
            }
            while (szSpacePos == szOldPos);
        }
    }

    std::cout << (cgl_context_sharing_supported ? "CL-GL sharing supported" : "CL-GL sharing NOT supported") << "\n";
}

/* Processing steps */

void OpenClParticleSimulator::runPopulateVoxelGridKernel(float dt_seconds, std::vector<cl_event> &events) {

}

void OpenClParticleSimulator::runCalculateParticleDensitiesKernel(float dt_seconds, std::vector<cl_event> &events) {

}

void OpenClParticleSimulator::runCalculateParticleForcesAndIntegrateStatesKernel(float dt_seconds,
                                                                                 std::vector<cl_event> &events) {

}

void OpenClParticleSimulator::runCopyParticlesToOpenGlBufferKernel(float dt_seconds, std::vector<cl_event> &events) {

}

/* Simple integrate positions kernel */
void OpenClParticleSimulator::runSimpleIntegratePositionsKernel(float dt_seconds, std::vector<cl_event> &events) {
    cl_int error = CL_SUCCESS;
    cl_event event;

    error = clEnqueueAcquireGLObjects(command_queue, 1, &cl_positions, 0, NULL, NULL);
    CheckError(error);
    error = clEnqueueAcquireGLObjects(command_queue, 1, &cl_velocities, 0, NULL, NULL);
    CheckError(error);

    // error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &cl_dt_obj);
    // CheckError(error);
    error = clSetKernelArg(simple_integration, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 2, sizeof(float), (void *) &dt_seconds);
    CheckError(error);

    error = clEnqueueNDRangeKernel(command_queue, simple_integration, 1, NULL, (const size_t *) &n_particles, NULL, 0,
                                   0, 0);
    //error = clEnqueueTask(command_queue, kernel, 0, NULL, event);
    CheckError(error);

    error = clEnqueueReleaseGLObjects(command_queue, 1, &cl_positions, 0, NULL, NULL);
    CheckError(error);
    error = clEnqueueReleaseGLObjects(command_queue, 1, &cl_velocities, 0, NULL, &event);
    CheckError(error);

    events.push_back(event);
}