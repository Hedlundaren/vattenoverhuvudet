#include "OpenCL/OpenClParticleSimulator.hpp"

#include <cmath>

#include "OpenCL/opencl_context_info.hpp"

#include "common/FileReader.hpp"

#include "OpenCL/clVoxelCell.hpp"
#include "Parameters.h"

#include "common/tic_toc.hpp"

void Exit() {
    std::exit(1);
}

OpenClParticleSimulator::~OpenClParticleSimulator() {
    // TODO clean up allocated space on the GPU (clRelease[...] ?)
}

void OpenClParticleSimulator::createAndBuildKernel(cl_kernel &kernel_out, std::string kernel_name,
                                                   std::string kernel_file_name) {
    //std::cout << "Creating kernel \"" << kernel_name << "\" from file kernels/" << kernel_file_name << ".\n\n";
    const auto kernel_str = FileReader::ReadFromFile("../kernels/" + kernel_file_name);

    const char *kernel_cstr = kernel_str.c_str();
    size_t kernel_str_size = std::strlen(kernel_str.c_str());
    //std::cout << kernel_str << "\n" << "Kernel program length = " << kernel_str_size << "\n";

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

    cgl_objects.push_back(cl_positions);
    cgl_objects.push_back(cl_velocities);

    // Particle counter
    cl_utility_particle_counter = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint), NULL, &error);
    CheckError(error);
    error = clRetainMemObject(cl_utility_particle_counter);
    CheckError(error);
}

void OpenClParticleSimulator::allocateVoxelGridBuffer() {
    using namespace Parameters;
    cl_int error = CL_SUCCESS;

    const unsigned int grid_size_x = static_cast<unsigned int>(ceilf(get_volume_size_x() / kernelSize));
    const unsigned int grid_size_y = static_cast<unsigned int>(ceilf(get_volume_size_y() / kernelSize));;
    const unsigned int grid_size_z = static_cast<unsigned int>(ceilf(get_volume_size_z() / kernelSize));;

    cl_voxel_grid_cell_count.s[0] = grid_size_x + 2;
    cl_voxel_grid_cell_count.s[1] = grid_size_y + 2;
    cl_voxel_grid_cell_count.s[2] = grid_size_z + 2;

    grid_cell_count = (grid_size_x + 2) * (grid_size_y + 2) * (grid_size_z + 2);

    //std::cout << "Grid size [x=" << grid_size_x << " y=" << grid_size_y << " z=" << grid_size_z <<
    //"], total cells = " << grid_cell_count << "\n";

    std::vector<clVoxelCell> voxel_grid_cells(grid_cell_count);

    clVoxelCell &cell = voxel_grid_cells[0];
    clParticle &p = cell.particles[3];

    // Just to check that all voxel cell particles are initialized to zero
    std::cout << print_clParticle(p);

    /*
    cl_voxel_grid = clCreateBuffer(context,
                                   CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                   grid_cell_count * sizeof(clVoxelCell),
                                   (void *) voxel_grid_cells.data(),
                                   &error);
                                   */
    cl_voxel_grid = clCreateBuffer(context,
                                   CL_MEM_READ_WRITE,
                                   grid_cell_count * sizeof(clVoxelCell),
                                   NULL,
                                   &error);
    CheckError(error);
    error = clEnqueueWriteBuffer(command_queue, cl_voxel_grid, CL_TRUE, 0, grid_cell_count * sizeof(clVoxelCell),
                                 (const void *) voxel_grid_cells.data(), 0, NULL, NULL);
    error = clRetainMemObject(cl_voxel_grid);
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
    createAndBuildKernel(populate_voxel_grid, "populate_voxel_grid", "populate_voxel_grid.cl");
    createAndBuildKernel(move_particles_to_ogl, "move_particles_to_ogl", "move_particles_to_ogl.cl");
}

void OpenClParticleSimulator::updateSimulation(float dt_seconds) {
    // Make sure all OpenGL commands will run before enqueueing OpenCL kernels
    glFlush();

    //std::vector<cl_event> events;
    cl_event event;
    cl_int error;

    tic();
    error = clEnqueueAcquireGLObjects(command_queue, cgl_objects.size(), (const cl_mem *) cgl_objects.data(),
                                      0, NULL, NULL);
    CheckError(error);

    runPopulateVoxelGridKernel(dt_seconds);
    //runCalculateParticleDensitiesKernel(dt_seconds, events);
    //runSimpleIntegratePositionsKernel(dt_seconds);
    //runCalculateParticleForcesAndIntegrateStatesKernel(dt_seconds, events);
    runMoveParticlesToOpenGlBufferKernel(dt_seconds);

    error = clEnqueueReleaseGLObjects(command_queue, (cl_uint) cgl_objects.size(), (const cl_mem *) cgl_objects.data(),
                                      0, NULL, &event);
    CheckError(error);

    //clWaitForEvents(1, &event);
    clFinish(command_queue);
    toc();
}

void OpenClParticleSimulator::initOpenCL() {
    cl_uint platformIdCount = 0;
    clGetPlatformIDs(0, NULL, &platformIdCount);


    if (platformIdCount == 0) {
        std::cerr << "No OpenCL platform found" << std::endl;
        Exit();
    } else {
        std::cout << "Found " << platformIdCount << " platform(s)" << std::endl;
    }

    // Resize vector to actual amount of platform ids and get the ids
    platformIds.resize(platformIdCount);
    clGetPlatformIDs(platformIdCount, platformIds.data(), NULL);

    for (cl_uint i = 0; i < platformIdCount; ++i) {
        std::cout << "\t (" << (i + 1) << ") : " << GetPlatformName(platformIds[i]) << std::endl;
    }

    cl_uint deviceIdCount = 0;
    clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, 0, NULL,
                   &deviceIdCount);

    if (deviceIdCount == 0) {
        std::cerr << "No OpenCL devices found" << std::endl;
        Exit();
    } else {
        std::cout << "Found " << deviceIdCount << " device(s)" << std::endl;
    }

    deviceIds.resize(deviceIdCount);
    clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_ALL, deviceIdCount,
                   deviceIds.data(), NULL);

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
                              deviceIds.data(), NULL, NULL, &error);
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

void OpenClParticleSimulator::runPopulateVoxelGridKernel(float dt_seconds) {
    std::cout << ">> populate_voxel_grid\n";

    cl_int error = CL_SUCCESS;

    cl_float3 voxel_grid_origin_corner = Parameters::get_volume_origin_corner_cl();
    cl_float voxel_grid_cell_dimensions = Parameters::kernelSize;

    std::cout << "  voxel_grid_cell_dimensions = " << voxel_grid_cell_dimensions << "\n";

    error = clSetKernelArg(populate_voxel_grid, 0, sizeof(cl_mem), (void *) &cl_voxel_grid);
    CheckError(error);
    error = clSetKernelArg(populate_voxel_grid, 1, sizeof(cl_float3), (void *) &voxel_grid_origin_corner);
    CheckError(error);
    error = clSetKernelArg(populate_voxel_grid, 2, sizeof(cl_float), (void *) &voxel_grid_cell_dimensions);
    CheckError(error);
    error = clSetKernelArg(populate_voxel_grid, 3, sizeof(cl_int3), (void *) &cl_voxel_grid_cell_count);
    CheckError(error);
    error = clSetKernelArg(populate_voxel_grid, 4, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(populate_voxel_grid, 5, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);

    std::cout << "  global_work_size = " << (const size_t) n_particles << "\n";

    error = clEnqueueNDRangeKernel(command_queue, populate_voxel_grid, 1, NULL, (const size_t *) &n_particles, NULL, 0,
                                   0, 0);
    CheckError(error);

    // Check if everything was correct
    std::vector<clVoxelCell> voxel_grid_cells(grid_cell_count);

    error = clEnqueueReadBuffer(command_queue, cl_voxel_grid, CL_TRUE, 0, grid_cell_count * sizeof(clVoxelCell),
                                (void *) voxel_grid_cells.data(), 0,
                                NULL, NULL);
    clFlush(command_queue);
    int count = 0;
    std::cout << "  voxel grid cell particle count =[";
    for (clVoxelCell cell : voxel_grid_cells) {
        std::cout << cell.particle_count << " ";
        count += cell.particle_count;
    }
    std::cout << "] total=" << count << "\n";
}

void OpenClParticleSimulator::runCalculateParticleDensitiesKernel(float dt_seconds) {

}

void OpenClParticleSimulator::runCalculateParticleForcesAndIntegrateStatesKernel(float dt_seconds) {

}

void OpenClParticleSimulator::runMoveParticlesToOpenGlBufferKernel(float dt_seconds) {
    //std::cout << ">> move_particles_to_ogl\n";
    cl_int error = CL_SUCCESS;

    // Reset the particle counter to zero
    const cl_uint zero = 0;
    error = clEnqueueWriteBuffer(command_queue, cl_utility_particle_counter, CL_TRUE, 0, sizeof(cl_uint),
                                 (const void *) &zero, 0, NULL, NULL);
    CheckError(error);

    error = clSetKernelArg(move_particles_to_ogl, 0, sizeof(cl_mem), (void *) &cl_voxel_grid);
    CheckError(error);
    error = clSetKernelArg(move_particles_to_ogl, 1, sizeof(cl_int3), (void *) &cl_voxel_grid_cell_count);
    CheckError(error);
    error = clSetKernelArg(move_particles_to_ogl, 2, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(move_particles_to_ogl, 3, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(move_particles_to_ogl, 4, sizeof(cl_mem), (void *) &cl_utility_particle_counter);
    CheckError(error);

    size_t global_work_sizes[3];
    global_work_sizes[0] = static_cast<size_t>(cl_voxel_grid_cell_count.s[0]);
    global_work_sizes[1] = static_cast<size_t>(cl_voxel_grid_cell_count.s[1]);
    global_work_sizes[2] = static_cast<size_t>(cl_voxel_grid_cell_count.s[2]);

    //std::cout << "  global_work_sizes = [" << global_work_sizes[0] << " " << global_work_sizes[1] << " " <<
    //global_work_sizes[2] << "]\n";

    error = clEnqueueNDRangeKernel(command_queue, move_particles_to_ogl, 3, NULL,
                                   (const size_t *) &global_work_sizes[0], NULL, 0, 0, 0);

    CheckError(error);

    cl_uint particle_counter = 0;
    clEnqueueReadBuffer(command_queue, cl_utility_particle_counter, CL_TRUE, 0, sizeof(cl_uint),
                        (void *) &particle_counter, 0, NULL, NULL);
    //std::cout << "  particle count = " << particle_counter << "\n";
}

/* Simple integrate positions kernel */
void OpenClParticleSimulator::runSimpleIntegratePositionsKernel(float dt_seconds) {
    std::cout << ">> taskParallelIntegrateVelocity\n";
    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(simple_integration, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 2, sizeof(float), (void *) &dt_seconds);
    CheckError(error);

    std::cout << "  global_work_size = " << (const size_t) n_particles << "\n";

    error = clEnqueueNDRangeKernel(command_queue, simple_integration, 1, NULL, (const size_t *) &n_particles,
                                   NULL, 0,
                                   NULL, NULL);
    CheckError(error);
}