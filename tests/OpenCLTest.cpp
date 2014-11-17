/*
 * OpenCLTest.cpp
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#ifdef HAVE_LIB_OPENCL

#define NUM_PARTICLES 20000

#include "OpenCLTest.h"
#include "TestUtils.h"

#include <cppunit/TestAssert.h>

#include <utility>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include <Rendering/CL/Event.h>
#include <Rendering/CL/Buffer.h>
#include <Rendering/CL/BufferGL.h>
#include <Rendering/CL/CommandQueue.h>
#include <Rendering/CL/Context.h>
#include <Rendering/CL/Device.h>
#include <Rendering/CL/Kernel.h>
#include <Rendering/CL/Platform.h>
#include <Rendering/CL/Program.h>

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Draw.h>
#include <Rendering/DrawCompound.h>
#include <Rendering/Helper.h>
#include <Rendering/BufferObject.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttribute.h>
#include <Rendering/Mesh/VertexAttributeIds.h>

#include <Util/Utils.h>
#include <Util/Graphics/ColorLibrary.h>

#include <Geometry/Vec4.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Angle.h>


CPPUNIT_TEST_SUITE_REGISTRATION(OpenCLTest);

const char* oclErrorString(cl_int error)
{
    static const char* errorString[] = {
        "CL_SUCCESS",
        "CL_DEVICE_NOT_FOUND",
        "CL_DEVICE_NOT_AVAILABLE",
        "CL_COMPILER_NOT_AVAILABLE",
        "CL_MEM_OBJECT_ALLOCATION_FAILURE",
        "CL_OUT_OF_RESOURCES",
        "CL_OUT_OF_HOST_MEMORY",
        "CL_PROFILING_INFO_NOT_AVAILABLE",
        "CL_MEM_COPY_OVERLAP",
        "CL_IMAGE_FORMAT_MISMATCH",
        "CL_IMAGE_FORMAT_NOT_SUPPORTED",
        "CL_BUILD_PROGRAM_FAILURE",
        "CL_MAP_FAILURE",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "CL_INVALID_VALUE",
        "CL_INVALID_DEVICE_TYPE",
        "CL_INVALID_PLATFORM",
        "CL_INVALID_DEVICE",
        "CL_INVALID_CONTEXT",
        "CL_INVALID_QUEUE_PROPERTIES",
        "CL_INVALID_COMMAND_QUEUE",
        "CL_INVALID_HOST_PTR",
        "CL_INVALID_MEM_OBJECT",
        "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
        "CL_INVALID_IMAGE_SIZE",
        "CL_INVALID_SAMPLER",
        "CL_INVALID_BINARY",
        "CL_INVALID_BUILD_OPTIONS",
        "CL_INVALID_PROGRAM",
        "CL_INVALID_PROGRAM_EXECUTABLE",
        "CL_INVALID_KERNEL_NAME",
        "CL_INVALID_KERNEL_DEFINITION",
        "CL_INVALID_KERNEL",
        "CL_INVALID_ARG_INDEX",
        "CL_INVALID_ARG_VALUE",
        "CL_INVALID_ARG_SIZE",
        "CL_INVALID_KERNEL_ARGS",
        "CL_INVALID_WORK_DIMENSION",
        "CL_INVALID_WORK_GROUP_SIZE",
        "CL_INVALID_WORK_ITEM_SIZE",
        "CL_INVALID_GLOBAL_OFFSET",
        "CL_INVALID_EVENT_WAIT_LIST",
        "CL_INVALID_EVENT",
        "CL_INVALID_OPERATION",
        "CL_INVALID_GL_OBJECT",
        "CL_INVALID_BUFFER_SIZE",
        "CL_INVALID_MIP_LEVEL",
        "CL_INVALID_GLOBAL_WORK_SIZE",
    };

    const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

    const int index = -error;

    return (index >= 0 && index < errorCount) ? errorString[index] : "";

}

const std::string hw("Hello World\n");

const char * hw_kernel = R"kernel(
	#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
	__constant char hw[] = "Hello World\n";
	__kernel void hello(__global char * out) {
		size_t tid = get_global_id(0);
		out[tid] = hw[tid];
	}
)kernel";

const char * particle_kernel = R"kernel(
    typedef struct {
        float4 pos;
        float4 col;
    } Vertex;

	__kernel void part2(__global Vertex* verts, __global float4* vel, __global float4* pos_gen, __global float4* vel_gen, float dt)
	{
		//get our index in the array
		unsigned int i = get_global_id(0);
		//copy position and velocity for this iteration to a local variable
		//note: if we were doing many more calculations we would want to have opencl
		//copy to a local memory array to speed up memory access (this will be the subject of a later tutorial)
		float4 p = verts[i].pos;
		float4 v = vel[i];
	
		//we've stored the life in the fourth component of our velocity array
		float life = vel[i].w;
		//decrease the life by the time step (this value could be adjusted to lengthen or shorten particle life
		life -= dt*2;
		//if the life is 0 or less we reset the particle's values back to the original values and set life to 1
		if(life <= 0)
		{
			p.xyz = pos_gen[i].xyz;
			v = vel_gen[i];
			life = 1.0;    
		}
	
		//we use a first order euler method to integrate the velocity and position (i'll expand on this in another tutorial)
		//update the velocity to be affected by "gravity" in the z direction
		v.y -= 9.8*dt;
		//update the position with the new velocity
		p.xyz += v.xyz*dt;
		//store the updated life in the velocity array
		v.w = life;
	
		//update the arrays with our newly computed values
		verts[i].pos = p;
		vel[i] = v;
	
		//you can manipulate the color based on properties of the system
		//here we adjust the alpha
		verts[i].col.w = life;
	
	}
)kernel";

//quick random function to distribute our initial points
float rand_float(float mn, float mx)
{
    float r = random() / (float) RAND_MAX;
    return mn + (mx-mn)*r;
}

Rendering::CL::Context createContext(uint32_t deviceType, bool shared) {

	using namespace Rendering;

	std::vector< CL::Platform > platformList;
	CL::Platform::get(&platformList);

	CPPUNIT_ASSERT(platformList.size() > 0);
	std::cerr << "\nPlatform number is: " << platformList.size() << std::endl;

	std::cerr << "Available platforms are:" << std::endl;
	CL::Platform platform;
	for(auto pf : platformList) {
		std::cerr << "  " << pf.getName() << " by " << pf.getVendor() << " (" << pf.getDevices().size() << " Devices)" << std::endl;
		for(auto dev : pf.getDevices()) {
			if(dev.getType() == deviceType)
				platform = pf;
		}
	}

	return CL::Context(platform, deviceType, shared);
}

void OpenCLTest::test() {
	using namespace Rendering;
	CL::Context context = createContext(CL::Device::TYPE_CPU, false);

	char* outH = new char[hw.length()-1];
	CL::Buffer outCL(context, hw.length()-1, CL::Buffer::WriteOnly, CL::Buffer::Use, outH);

	std::vector<CL::Device> devices = context.getDevices();
	CPPUNIT_ASSERT(devices.size() > 0);

	CL::Program program(context, hw_kernel);
	CPPUNIT_ASSERT(program.build(devices));

	CL::Kernel kernel(program, "hello");
	CPPUNIT_ASSERT(kernel.setArg(0, outCL));

	CL::CommandQueue queue(context, devices[0]);

	//cl::Event event;
	CPPUNIT_ASSERT(queue.execute(kernel, {0}, {hw.length()+1}, {1}));

	//event.wait();
	CPPUNIT_ASSERT(queue.read(outCL, 0, hw.length()-1, outH, true));

	std::cout << outH;
}

void OpenCLTest::interopTest() {
	using namespace Rendering;
	using namespace Geometry;
	using namespace Util;

	CL::Context context = createContext(CL::Device::TYPE_GPU, true);

    std::vector<CL::Device> devices = context.getDevices();
	CPPUNIT_ASSERT(devices.size() > 0);

	CL::Program program(context, particle_kernel);
	bool built = program.build(devices);
	std::cout << "Build Status: " << program.getBuildStatus(devices[0]) << std::endl;
	std::cout << "Build Options:\t" << program.getBuildOptions(devices[0]) << std::endl;
	std::cout << "Build Log:\t " << program.getBuildLog(devices[0]) << std::endl;
	CPPUNIT_ASSERT(built);

	CL::CommandQueue queue(context, devices[0]);

    size_t array_size; //the size of our arrays num * sizeof(Vec4)

	RenderingContext rc;
	rc.setImmediateMode(false);
	Rendering::disableGLErrorChecking();

	//initialize our particle system with positions, velocities and color
	int num = NUM_PARTICLES;
	std::vector<Vec4> posGen(num);
	std::vector<Vec4> vel(num);

	VertexDescription vd;
	vd.appendPosition4D();
	vd.appendColorRGBAFloat();
	MeshUtils::MeshBuilder mb(vd);

	//fill our vectors with initial data
	for(int i = 0; i < num; i++)
	{
		//distribute the particles in a random circle around z axis
		float rad = rand_float(.1, 0.3f);
		float x = rad*sin(2*3.14 * i/num);
		float z = 0.0f;// -.1 + .2f * i/num;
		float y = rad*cos(2*3.14 * i/num);
		posGen[i] = Vec4(x,y,z,1);

		//give some initial velocity
		float xr = rand_float(-1, 1);
		float yr = rand_float(1.f, 3.f);
		//the life is the lifetime of the particle: 1 = alive 0 = dead
		//as you will see in part2.cl we reset the particle when it dies
		float life_r = rand_float(0.f, 1.f);
		vel[i] = Vec4(xr, yr, 3.0f, life_r);

		mb.addVertex({x,y,z}, {}, 1,0,0,1,0,0);

	}
	std::unique_ptr<Mesh> mesh(mb.buildMesh());

	//store the number of particles and the size in bytes of our arrays
	num = mesh->getVertexCount();
	array_size = num * sizeof(Vec4);
	mesh->setGLDrawMode(GL_POINTS);
	mesh->setDataStrategy(SimpleMeshDataStrategy::getDynamicVertexStrategy());
	mesh->getDataStrategy()->prepare(mesh.get());
	//mesh->_getVertexData().upload(GL_DYNAMIC_DRAW);

	//make sure OpenGL is finished before we proceed
	rc.finish();

	// create OpenCL buffer from GL VBO
	CL::Buffer cl_vbo = CL::BufferGL(context, CL::Buffer::ReadWrite, mesh->_getVertexData()._getBufferId());

	//create the OpenCL only arrays
	CL::Buffer cl_velocities = CL::Buffer(context, array_size, CL::Buffer::WriteOnly);
	CL::Buffer cl_pos_gen = CL::Buffer(context, array_size, CL::Buffer::WriteOnly);
	CL::Buffer cl_vel_gen = CL::Buffer(context, array_size, CL::Buffer::WriteOnly);

    CL::Event event;
	//push our CPU arrays to the GPU
	//data is tightly packed in std::vector starting with the adress of the first element
    CPPUNIT_ASSERT(queue.write(cl_velocities, 0, array_size, &vel[0], true, &event));
    CPPUNIT_ASSERT(queue.write(cl_pos_gen, 0, array_size, &posGen[0], true, &event));
    CPPUNIT_ASSERT(queue.write(cl_vel_gen, 0, array_size, &vel[0], true, &event));
    queue.finish();

    //initialize our kernel from the program
	CL::Kernel kernel(program, "part2");
    CPPUNIT_ASSERT(kernel.setArg(0, cl_vbo)); //position vbo
    CPPUNIT_ASSERT(kernel.setArg(1, cl_velocities));
    CPPUNIT_ASSERT(kernel.setArg(2, cl_pos_gen));
    CPPUNIT_ASSERT(kernel.setArg(3, cl_vel_gen));

    //Wait for the command queue to finish these commands before proceeding
    queue.finish();

    rc.setViewport({0,0,256,256});
    rc.setMatrix_modelToCamera(Matrix4x4f::orthographicProjection(-1,1,-1,1,-100,100));

    rc.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::NEVER));
    rc.pushAndSetLighting(LightingParameters(false));
    rc.pushAndSetBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
    rc.pushAndSetPointParameters(PointParameters(2, true));

	for(uint_fast32_t round = 0; round < 1000; ++round) {
		rc.applyChanges();

		rc.clearScreen(ColorLibrary::WHITE);

		rc.finish();
		 // map OpenGL buffer object for writing from OpenCL
		//this passes in the vector of VBO buffer objects (position and color)
		queue.acquireGLObjects(cl_vbo, &event);
		queue.finish();

		float dt = .01f;
		kernel.setArg(4, dt); //pass in the timestep
		//execute the kernel
		queue.execute(kernel, {}, {num}, {}, &event);
		queue.finish();

		//Release the VBOs so OpenGL can play with them
		queue.releaseGLObjects(cl_vbo, &event);
		queue.finish();

		rc.displayMesh(mesh.get());

		TestUtils::window->swapBuffers();
	}
}

#endif /* HAVE_LIB_OPENCL */
