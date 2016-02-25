#include "OpenCL/OpenClParticleSimulator.hpp"

#include "OpenCL/opencl_context_info.hpp"

#include "common/FileReader.hpp"

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
}

void OpenClParticleSimulator::allocateVoxelGridBuffer(const Parameters &params) {
    cl_int error = CL_SUCCESS;

    clVoxelGridInfo grid_info;
    params.set_voxel_grid_info(grid_info);

    grid_cells_count = new size_t[3];
    grid_cells_count[0] = grid_info.grid_dimensions.s[0];
    grid_cells_count[1] = grid_info.grid_dimensions.s[1];
    grid_cells_count[2] = grid_info.grid_dimensions.s[2];

    /* Setup voxel cell particle indices */
    std::vector<cl_uint> voxel_cell_particle_indices_zeroes(
            grid_info.max_cell_particle_count * grid_info.total_grid_cells);

    cl_voxel_cell_particle_indices = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                                    voxel_cell_particle_indices_zeroes.size() * sizeof(cl_uint),
                                                    NULL, &error);
    CheckError(error);

    error = clEnqueueWriteBuffer(command_queue, cl_voxel_cell_particle_indices, CL_TRUE, 0,
                                 voxel_cell_particle_indices_zeroes.size() * sizeof(cl_uint),
                                 (const void *) voxel_cell_particle_indices_zeroes.data(),
                                 NULL, NULL, NULL);
    CheckError(error);
    error = clRetainMemObject(cl_voxel_cell_particle_indices);
    CheckError(error);

    cl_voxel_cell_particle_count = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                                  grid_info.total_grid_cells * sizeof(cl_uint),
                                                  NULL, &error);
    CheckError(error);
    error = clEnqueueWriteBuffer(command_queue, cl_voxel_cell_particle_count, CL_TRUE, 0,
                                 grid_info.total_grid_cells * sizeof(cl_uint),
                                 (const void *) voxel_cell_particle_indices_zeroes.data(),
                                 NULL, NULL, NULL);
    CheckError(error);
    error = clRetainMemObject(cl_voxel_cell_particle_count);
    CheckError(error);

    error = clFlush(command_queue);
    CheckError(error);

    /* Setup density calculation buffer */
    std::vector<cl_float> voxel_cell_particle_densities_zeroes(
            grid_info.max_cell_particle_count * grid_info.total_grid_cells);

    cl_densities = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                  voxel_cell_particle_densities_zeroes.size() * sizeof(cl_float),
                                  NULL, &error);
    CheckError(error);
    error = clEnqueueWriteBuffer(command_queue, cl_densities, CL_TRUE, 0,
                                 voxel_cell_particle_densities_zeroes.size() * sizeof(cl_float),
                                 (const void *) voxel_cell_particle_densities_zeroes.data(),
                                 NULL, NULL, NULL);
    CheckError(error);
    error = clRetainMemObject(cl_densities);
    CheckError(error);

    /* Setup force calculation buffer */
    std::vector<cl_float3> particle_forces_zeroes(n_particles);

    cl_forces = clCreateBuffer(context, CL_MEM_READ_WRITE,
                               particle_forces_zeroes.size() * sizeof(cl_float3),
                               NULL, &error);
    CheckError(error);
    error = clEnqueueWriteBuffer(command_queue, cl_forces, CL_TRUE, 0,
                                 particle_forces_zeroes.size() * sizeof(cl_float3),
                                 (const void *) particle_forces_zeroes.data(),
                                 NULL, NULL, NULL);
    CheckError(error);
    error = clRetainMemObject(cl_forces);
    CheckError(error);
}

void OpenClParticleSimulator::setupSimulation(const Parameters &params,
                                              const std::vector<glm::vec3> &particle_positions,
                                              const std::vector<glm::vec3> &particle_velocities,
                                              const GLuint &vbo_positions,
                                              const GLuint &vbo_velocities) {
    positions = particle_positions;

    initOpenCL();
    std::cout << "\nOpenCL ready to use: context created.\n\n";

    n_particles = particle_positions.size();


    // Here we can use OpenCL functionality
    // cl_int error = CL_SUCCESS;

    setupSharedBuffers(vbo_positions, vbo_velocities);
    allocateVoxelGridBuffer(params);

    createAndBuildKernel(simple_integration, "taskParallelIntegrateVelocity", "update_particle_positions.cl");
    createAndBuildKernel(calculate_voxel_grid, "calculate_voxel_grid", "calculate_voxel_grid.cl");
    createAndBuildKernel(reset_voxel_grid, "reset_voxel_grid", "calculate_voxel_grid.cl");
    createAndBuildKernel(simple_voxel_grid_move, "simple_voxel_grid_move", "simple_voxel_grid_move.cl");
    createAndBuildKernel(calculate_particle_densities, "calculate_particle_densities", "simulate_fluid_particles.cl");
    createAndBuildKernel(calculate_particle_forces, "calculate_forces", "simulate_fluid_particles.cl");
    createAndBuildKernel(integrate_particle_states, "integrate_particle_states", "integrate_particle_states.cl");
}

void OpenClParticleSimulator::updateSimulation(const Parameters &parameters, float dt_seconds) {
    parameters.set_voxel_grid_info(grid_info);
    parameters.set_fluid_info(fluid_info, parameters.n_particles);
    n_particles = parameters.n_particles;

    // Make sure all OpenGL commands will run before enqueueing OpenCL kernels
    glFlush();

    //std::vector<cl_event> events;
    cl_event event;
    cl_int error;

#ifdef MY_DEBUG
    tic();
#endif
    error = clEnqueueAcquireGLObjects(command_queue, cgl_objects.size(), (const cl_mem *) cgl_objects.data(),
                                      0, NULL, NULL);
    CheckError(error);

    runCalculateVoxelGridKernel(dt_seconds);
    runCalculateParticleDensitiesKernel(dt_seconds);
    runCalculateParticleForcesKernel();
    runResetVoxelGridKernel();
    runIntegrateParticleStatesKernel(dt_seconds);

    error = clEnqueueReleaseGLObjects(command_queue, (cl_uint) cgl_objects.size(), (const cl_mem *) cgl_objects.data(),
                                      0, NULL, &event);
    CheckError(error);

    //clWaitForEvents(1, &event);
    clFinish(command_queue);
#ifdef MY_DEBUG
    toc();
#endif
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

void OpenClParticleSimulator::runCalculateVoxelGridKernel(float dt_seconds) {
#ifdef MY_DEBUG
    std::cout << ">> calculate_voxel_grid\n";
    std::cout << "  " << print_clVoxelGridInfo(grid_info) << "\n";
#endif

    cl_int error = CL_SUCCESS;


    error = clSetKernelArg(calculate_voxel_grid, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(calculate_voxel_grid, 1, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_indices);
    CheckError(error);
    error = clSetKernelArg(calculate_voxel_grid, 2, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_count);
    CheckError(error);
    error = clSetKernelArg(calculate_voxel_grid, 3, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);

    error = clEnqueueNDRangeKernel(command_queue, calculate_voxel_grid, 1, NULL, (const size_t *) &n_particles, NULL,
                                   NULL, NULL, NULL);
    CheckError(error);

    /*
     * Read back data and check for correctness
     */

#ifdef MY_DEBUG
    cl_uint voxel_cell_particle_count[grid_info.total_grid_cells];
    error = clEnqueueReadBuffer(command_queue, cl_voxel_cell_particle_count, CL_TRUE, 0,
                                grid_info.total_grid_cells * sizeof(cl_uint), (void *) &voxel_cell_particle_count[0],
                                0, NULL, NULL);

    unsigned int total = 0;
    for (unsigned int i = 0; i < grid_info.total_grid_cells; ++i) {
        total += voxel_cell_particle_count[i];
    }
    std::cout << "  voxel_cell_particle_count (count) = " << total << "\n    ";

    const unsigned int PARTICLE_INDICES_COUNT = grid_info.total_grid_cells * grid_info.max_cell_particle_count;
    cl_uint voxel_cell_particle_indices[PARTICLE_INDICES_COUNT];
    error = clEnqueueReadBuffer(command_queue, cl_voxel_cell_particle_indices, CL_TRUE, 0,
                                grid_info.total_grid_cells * grid_info.max_cell_particle_count * sizeof(cl_uint),
                                (void *) &voxel_cell_particle_indices[0],
                                0, NULL, NULL);
    total = 0;
    for (unsigned int i = 0; i < PARTICLE_INDICES_COUNT; ++i) {
        if (voxel_cell_particle_indices[i] != 0) {
            total++;
        }
    }
    std::cout << "  voxel_cell_particle_indices (count) = " << total + 1 << "\n    ";
#endif
}

void OpenClParticleSimulator::runResetVoxelGridKernel() {
#ifdef MY_DEBUG
    std::cout << ">> reset_voxel_grid\n";
#endif

    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(reset_voxel_grid, 0, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_indices);
    CheckError(error);
    error = clSetKernelArg(reset_voxel_grid, 1, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_count);
    CheckError(error);
    error = clSetKernelArg(reset_voxel_grid, 2, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);

    const size_t total_grid_cells = static_cast<size_t>(grid_info.total_grid_cells);
    error = clEnqueueNDRangeKernel(command_queue, reset_voxel_grid, 1, NULL,
                                   (const size_t *) &total_grid_cells, NULL,
                                   NULL, NULL, NULL);
    CheckError(error);

    /* Read back data and check for correctness */

#ifdef MY_DEBUG
    const unsigned int PARTICLE_COUNT = grid_info.total_grid_cells;
    cl_uint voxel_cell_particle_count[PARTICLE_COUNT];
    error = clEnqueueReadBuffer(command_queue, cl_voxel_cell_particle_count, CL_TRUE, 0,
                                grid_info.total_grid_cells * sizeof(cl_uint), (void *) &voxel_cell_particle_count[0],
                                0, NULL, NULL);

    unsigned int total = 0;
    for (unsigned int i = 0; i < PARTICLE_COUNT; ++i) {
        total += voxel_cell_particle_count[i];
    }
    std::cout << "  [post-reset] voxel_cell_particle_count (count) = " << total << "\n    ";

    const unsigned int PARTICLE_INDICES_COUNT = grid_info.total_grid_cells * grid_info.max_cell_particle_count;
    cl_uint voxel_cell_particle_indices[PARTICLE_INDICES_COUNT];
    error = clEnqueueReadBuffer(command_queue, cl_voxel_cell_particle_indices, CL_TRUE, 0,
                                grid_info.total_grid_cells * grid_info.max_cell_particle_count * sizeof(cl_uint),
                                (void *) &voxel_cell_particle_indices[0],
                                0, NULL, NULL);
    total = 0;
    for (unsigned int i = 0; i < PARTICLE_INDICES_COUNT; ++i) {
        if (voxel_cell_particle_indices[i] != 0) {
            total++;
        }
    }

    std::cout << "  [post-reset] voxel_cell_particle_indices (count) = " << total << "\n    ";
#endif
}

void OpenClParticleSimulator::runSimpleVoxelGridMoveKernel(float dt_seconds) {
#ifdef MY_DEBUG
    std::cout << ">> simple_voxel_grid_move\n";
#endif

    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(simple_voxel_grid_move, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(simple_voxel_grid_move, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(simple_voxel_grid_move, 2, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_indices);
    CheckError(error);
    error = clSetKernelArg(simple_voxel_grid_move, 3, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_count);
    CheckError(error);
    error = clSetKernelArg(simple_voxel_grid_move, 4, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);
    const cl_float cl_dt = dt_seconds;
    error = clSetKernelArg(simple_voxel_grid_move, 5, sizeof(cl_float), (void *) &cl_dt);
    CheckError(error);

    error = clEnqueueNDRangeKernel(command_queue, simple_voxel_grid_move, 1, NULL,
                                   (const size_t *) &grid_info.total_grid_cells, NULL,
                                   NULL, NULL, NULL);
    CheckError(error);
}

void OpenClParticleSimulator::runCalculateParticleDensitiesKernel(float dt_seconds) {
#ifdef MY_DEBUG
    std::cout << ">> calculate_particle_densities\n";
#endif

    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(calculate_particle_densities, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_densities, 1, sizeof(cl_mem), (void *) &cl_densities);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_densities, 2, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_indices);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_densities, 3, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_count);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_densities, 4, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_densities, 5, sizeof(clFluidInfo), (void *) &fluid_info);
    CheckError(error);

    error = clEnqueueNDRangeKernel(command_queue, calculate_particle_densities, 3, NULL,
                                   (const size_t *) grid_cells_count, NULL,
                                   NULL, NULL, NULL);
    CheckError(error);

#ifdef MY_DEBUG
    const unsigned int PARTICLE_DENSITIES_COUNT = grid_info.max_cell_particle_count * grid_info.total_grid_cells;
    cl_float particle_densities[PARTICLE_DENSITIES_COUNT];
    error = clEnqueueReadBuffer(command_queue, cl_densities, CL_TRUE, 0,
                                PARTICLE_DENSITIES_COUNT * sizeof(cl_float),
                                (void *) &particle_densities[0],
                                0, NULL, NULL);
    CheckError(error);
    clFinish(command_queue);

    unsigned int zero_counter = 0;
    unsigned int NaN_counter = 0;

    std::cout << "Densities: \n";
    for (unsigned int i = 0; i < PARTICLE_DENSITIES_COUNT; ++i) {
        auto density = particle_densities[i];

        if (density < 0.0001f) {
            ++zero_counter;
        }
        if (isnan(density)) {
            ++NaN_counter;
        }
        if (density > 0.0001f && !isnan(density)) {
            std::cout << density << ", ";
        }
    }
    std::cout << "\nNaN-count = " << NaN_counter << ", zero-count = " << zero_counter << "\n";

    std::cout << "\n";
#endif
}

void OpenClParticleSimulator::runCalculateParticleForcesKernel() {
#ifdef MY_DEBUG
    std::cout << ">> calculate_particle_forces\n";
#endif

    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(calculate_particle_forces, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 2, sizeof(cl_mem), (void *) &cl_forces);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 3, sizeof(cl_mem), (void *) &cl_densities);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 4, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_indices);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 5, sizeof(cl_mem), (void *) &cl_voxel_cell_particle_count);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 6, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);
    error = clSetKernelArg(calculate_particle_forces, 7, sizeof(clFluidInfo), (void *) &fluid_info);
    CheckError(error);

    error = clFinish(command_queue);
    CheckError(error);

    error = clEnqueueNDRangeKernel(command_queue, calculate_particle_forces, 3, NULL,
                                   (const size_t *) grid_cells_count, NULL,
                                   NULL, NULL, NULL);
    CheckError(error);

#ifdef MY_DEBUG
    const unsigned int PARTICLE_FORCES_COUNT = n_particles;
    cl_float3 particle_forces[PARTICLE_FORCES_COUNT];
    error = clEnqueueReadBuffer(command_queue, cl_forces, CL_TRUE, 0,
                                PARTICLE_FORCES_COUNT * sizeof(cl_float3),
                                (void *) &particle_forces[0],
                                0, NULL, NULL);
    CheckError(error);

    unsigned int counter = 0;
    for (unsigned int i = 0; i < PARTICLE_FORCES_COUNT; ++i) {
        auto force = particle_forces[i];

        if (isnan(force.s[0]) || isnan(force.s[1]) || isnan(force.s[2])) {
            ++counter;
        }
        //if (density > 0.0f) {
        std::cout << "[" << force.s[0] << " " << force.s[1] << " " << force.s[2] << "], ";
        //}
    }

    std::cout << "\n";
    std::cout << "NaN-count = " << counter << "\n";
#endif
}

/* Simple integrate positions kernel */
void OpenClParticleSimulator::runSimpleIntegratePositionsKernel(float dt_seconds) {
#ifdef MY_DEBUG
    std::cout << ">> taskParallelIntegrateVelocity\n";
#endif
    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(simple_integration, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(simple_integration, 2, sizeof(float), (void *) &dt_seconds);
    CheckError(error);

#ifdef MY_DEBUG
    std::cout << "  global_work_size = " << (const size_t) n_particles << "\n";
#endif

    error = clEnqueueNDRangeKernel(command_queue, simple_integration, 1, NULL, (const size_t *) &n_particles,
                                   NULL, 0,
                                   NULL, NULL);
    CheckError(error);
}

void OpenClParticleSimulator::runIntegrateParticleStatesKernel(float dt_seconds) {
#ifdef MY_DEBUG
    std::cout << ">> integrate_particle_states\n";
#endif
    cl_int error = CL_SUCCESS;

    error = clSetKernelArg(integrate_particle_states, 0, sizeof(cl_mem), (void *) &cl_positions);
    CheckError(error);
    error = clSetKernelArg(integrate_particle_states, 1, sizeof(cl_mem), (void *) &cl_velocities);
    CheckError(error);
    error = clSetKernelArg(integrate_particle_states, 2, sizeof(cl_mem), (void *) &cl_forces);
    CheckError(error);
    error = clSetKernelArg(integrate_particle_states, 3, sizeof(clVoxelGridInfo), (void *) &grid_info);
    CheckError(error);
    error = clSetKernelArg(integrate_particle_states, 4, sizeof(clFluidInfo), (void *) &fluid_info);
    CheckError(error);
    error = clSetKernelArg(integrate_particle_states, 5, sizeof(float), (void *) &dt_seconds);
    CheckError(error);

#ifdef MY_DEBUG
    std::cout << "  global_work_size = " << (const size_t) n_particles << "\n";
#endif

    error = clEnqueueNDRangeKernel(command_queue, integrate_particle_states, 1, NULL, (const size_t *) &n_particles,
                                   NULL, 0,
                                   NULL, NULL);
    CheckError(error);
}