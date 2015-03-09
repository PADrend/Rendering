/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL

#define NUM_PARTICLES 20000

#include "OpenCLTest.h"
#include "TestUtils.h"

#include <Util/Macros.h>

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
COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wpedantic)
COMPILER_WARN_OFF(-Wold-style-cast)
COMPILER_WARN_OFF(-Wcast-qual)
COMPILER_WARN_OFF(-Wshadow)
COMPILER_WARN_OFF(-Wstack-protector)
#include <CL/cl.hpp>
COMPILER_WARN_POP

#include <Rendering/CL/Event.h>
#include <Rendering/CL/Memory/Buffer.h>
#include <Rendering/CL/Memory/BufferAccessor.h>
#include <Rendering/CL/Memory/Image.h>
#include <Rendering/CL/CommandQueue.h>
#include <Rendering/CL/Context.h>
#include <Rendering/CL/Device.h>
#include <Rendering/CL/Kernel.h>
#include <Rendering/CL/Platform.h>
#include <Rendering/CL/Program.h>
#include <Rendering/CL/UserEvent.h>
#include <Rendering/CL/CLUtils.h>

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
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Util/Utils.h>
#include <Util/References.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/BitmapUtils.h>
#include <Util/Graphics/PixelAccessor.h>

#include <Geometry/Vec4.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Angle.h>


CPPUNIT_TEST_SUITE_REGISTRATION(OpenCLTest);

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

const char* simple_filter = R"kernel(
	__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
	
	float filterValue (__constant const float* filterWeights, const int x, const int y) {
		return filterWeights[(x+FILTER_SIZE) + (y+FILTER_SIZE)*(FILTER_SIZE*2 + 1)];
	}
	
	__kernel void filter (__read_only image2d_t input, __constant float* filterWeights, __write_only image2d_t output) {
		const int2 pos = {get_global_id(0), get_global_id(1)};
	
		float4 sum = (float4)(0.0f);
		for(int y = -FILTER_SIZE; y <= FILTER_SIZE; y++) {
			for(int x = -FILTER_SIZE; x <= FILTER_SIZE; x++) {
				sum += filterValue(filterWeights, x, y)
					* read_imagef(input, sampler, pos + (int2)(x,y));
			}
		}
	
		write_imagef (output, (int2)(pos.x, pos.y), sum);
	}
)kernel";

//quick random function to distribute our initial points
inline
float rand_float(float mn, float mx)
{
    float r = random() / (float) RAND_MAX;
    return mn + (mx-mn)*r;
}

void OpenCLTest::test() {
	using namespace Util;
	using namespace Rendering;
	
	std::vector<CL::PlatformRef> platformList = CL::Platform::get();

	std::cout << std::endl << "Available Platforms:" << std::endl;
	for(auto pf : platformList) {
		auto devices = pf->getDevices();
		std::cout << "\t" << pf->getName() << " (" << devices.size() << " Devices):" << std::endl;
		for(auto dev : devices) {
			std::cout << "\t\t" << dev->getName() << std::endl;
		}
	}
	
	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get());
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());
	CL::ProgramRef program = new CL::Program(context.get(), {hw_kernel});
	CPPUNIT_ASSERT(program->build({device}));

	char* outH = new char[hw.length()-1];
	CL::BufferRef outCL = new CL::Buffer(context.get(), hw.length()-1, CL::ReadWrite_t::WriteOnly, CL::HostPtr_t::Use, outH);

	CL::KernelRef kernel = new CL::Kernel(program.get(), "hello");
	CPPUNIT_ASSERT(kernel->setArg(0, outCL.get()));

	CPPUNIT_ASSERT(queue->execute(kernel.get(), {0}, {hw.length()+1}, {1}, {}));
	queue->finish();
	CPPUNIT_ASSERT(queue->readBuffer(outCL.get(), true, 0, hw.length()-1, outH));
	queue->finish();

	std::cout << outH;
}

void OpenCLTest::interopTest() {
	using namespace Rendering;
	using namespace Geometry;
	using namespace Util;

	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_GPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get(), true);
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());
	CL::ProgramRef program = new CL::Program(context.get(), {particle_kernel});
	CPPUNIT_ASSERT(program->build({device}));

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
	BufferObject vbo;
	mesh->_getVertexData()._swapBufferObject(vbo);
	CL::BufferRef cl_vbo = new CL::Buffer(context.get(), CL::ReadWrite_t::ReadWrite, vbo);
	mesh->_getVertexData()._swapBufferObject(vbo);

	//create the OpenCL only arrays
	CL::BufferRef cl_velocities = new CL::Buffer(context.get(), array_size, CL::ReadWrite_t::WriteOnly);
	CL::BufferRef cl_pos_gen = new CL::Buffer(context.get(), array_size, CL::ReadWrite_t::WriteOnly);
	CL::BufferRef cl_vel_gen = new CL::Buffer(context.get(), array_size, CL::ReadWrite_t::WriteOnly);

	//push our CPU arrays to the GPU
    CPPUNIT_ASSERT(queue->writeBuffer(cl_velocities.get(), true, 0, array_size, &vel[0]));
    CPPUNIT_ASSERT(queue->writeBuffer(cl_pos_gen.get(), true, 0, array_size, &posGen[0]));
    CPPUNIT_ASSERT(queue->writeBuffer(cl_vel_gen.get(), true, 0, array_size, &vel[0]));
    queue->finish();

    //initialize our kernel from the program
	CL::KernelRef kernel = new CL::Kernel(program.get(), "part2");
	CPPUNIT_ASSERT(kernel->setArgs(cl_vbo, cl_velocities, cl_pos_gen, cl_vel_gen));

    //Wait for the command queue to finish these commands before proceeding
    queue->finish();

    rc.setViewport({0,0,256,256});
    rc.setMatrix_modelToCamera(Matrix4x4f::orthographicProjection(-1,1,-1,1,-100,100));

    rc.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::NEVER));
    rc.pushAndSetLighting(LightingParameters(false));
    rc.pushAndSetBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
    rc.pushAndSetPointParameters(PointParameters(2, true));

    double time = 0;
	for(uint_fast32_t round = 0; round < 100; ++round) {
		rc.applyChanges();

		rc.clearScreen(ColorLibrary::WHITE);

		rc.finish();
		 // map OpenGL buffer object for writing from OpenCL
		//this passes in the vector of VBO buffer objects (position and color)
		queue->acquireGLObjects({cl_vbo.get()});
		queue->finish();

//	    CL::UserEvent userevent(context.get()); // user events seem to be broken for GL-CL interoperability (at least on nvidia)
	    CL::Event event;

		float dt = .01f;
		kernel->setArg(4, dt); //pass in the timestep
		//execute the kernel
		queue->execute(kernel.get(), {}, {num}, {}, {}, &event);
//		event.setCallback([=](const CL::Event& e, int32_t s){ std::cout << round << " ";});
		queue->finish();

		time += (event.getProfilingCommandEnd() - event.getProfilingCommandStart()) * 1.0e-6;

		//Release the VBOs so OpenGL can play with them
		queue->releaseGLObjects({cl_vbo.get()});
		queue->finish();

		rc.displayMesh(mesh.get());

		TestUtils::window->swapBuffers();
	}
	std::cout << std::endl << "Time: " << time << " ms (Avg: " << (time/1000) << " ms)"<< std::endl;
}

void OpenCLTest::textureGLFilterTest() {
	using namespace Rendering;
	using namespace Geometry;
	using namespace Util;

	// create rendering context
	RenderingContext rc;
	rc.setImmediateMode(false);

    rc.setViewport({0,0,256,256});
    rc.setMatrix_modelToCamera(Matrix4x4f::orthographicProjection(-1,1,-1,1,-100,100));
    rc.pushAndSetLighting(LightingParameters(false));

	// initialize OpenCL
	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_GPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get(), true);
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());
	CL::ProgramRef program = new CL::Program(context.get(), {simple_filter});
	CPPUNIT_ASSERT(program->build({device}, "-D FILTER_SIZE=1"));

	Reference<Texture> inTexture = TextureUtils::createChessTexture(256, 256, 32);
	Reference<Texture> outTexture = TextureUtils::createChessTexture(256, 256, 32);

	// Textures needs to be on the GPU for further processing
	inTexture->_prepareForBinding(rc);
	outTexture->_prepareForBinding(rc);

	//make sure OpenGL is finished before we proceed
	rc.applyChanges();
	rc.finish();

	CL::ImageRef inImage = new CL::Image(context.get(), CL::ReadWrite_t::ReadWrite, inTexture.get());
	CL::ImageRef outImage = new CL::Image(context.get(), CL::ReadWrite_t::WriteOnly, outTexture.get());

	// Simple Gaussian blur filter
	float filter [] = {
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	};

	// Normalize the filter
	for (int i = 0; i < 9; ++i) {
		filter [i] /= 16.0f;
	}
	CL::BufferRef filterBuffer = new CL::Buffer(context.get(), 9*sizeof(float), CL::ReadWrite_t::ReadOnly, CL::HostPtr_t::Copy, filter);

	CL::KernelRef kernel = new CL::Kernel(program.get(), "filter");
	CPPUNIT_ASSERT(kernel->setArgs(inImage, filterBuffer, outImage));

	CPPUNIT_ASSERT(queue->acquireGLObjects({inImage.get(), outImage.get()}));
	queue->finish();
	//execute the kernel
	CPPUNIT_ASSERT(queue->execute(kernel.get(), {}, {256, 256}, {}));
	queue->finish();
	CPPUNIT_ASSERT(queue->releaseGLObjects({inImage.get(), outImage.get()}));
	queue->finish();

	for(uint_fast32_t round = 0; round < 100; ++round) {
		rc.applyChanges();
		rc.clearScreen(ColorLibrary::BLACK);
	    TextureUtils::drawTextureToScreen(rc, {0,0,256,256}, round < 50 ? *inTexture.get() : *outTexture.get(), {0,0,1,1});
		TestUtils::window->swapBuffers();
	}
}

void OpenCLTest::bitmapFilterTest() {
	using namespace Rendering;
	using namespace Geometry;
	using namespace Util;

	// create rendering context
	RenderingContext rc;
	rc.setImmediateMode(false);

	rc.setViewport({0,0,256,256});
	rc.setMatrix_modelToCamera(Matrix4x4f::orthographicProjection(-1,1,-1,1,-100,100));
	rc.pushAndSetLighting(LightingParameters(false));

	// initialize OpenCL
	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get(), false);
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());
	CL::ProgramRef program = new CL::Program(context.get(), {simple_filter});
	CPPUNIT_ASSERT(program->build({device}, "-D FILTER_SIZE=1"));

	Reference<Texture> inTexture = TextureUtils::createChessTexture(256, 256, 32);
	Reference<Texture> outTexture = TextureUtils::createChessTexture(256, 256, 32);

	Reference<Bitmap> inBitmap = inTexture->getLocalBitmap();
	Reference<Bitmap> outBitmap = outTexture->getLocalBitmap();
	std::fill(outBitmap->data(), outBitmap->data() + outBitmap->getDataSize(), 0);

	CL::ImageRef inImage = new CL::Image(context.get(), CL::ReadWrite_t::ReadOnly, inBitmap.get());
	CL::ImageRef outImage = new CL::Image(context.get(), CL::ReadWrite_t::ReadWrite, outBitmap.get());

	// Simple Gaussian blur filter
	float filter [] = {
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	};

	// Normalize the filter
	for (int i = 0; i < 9; ++i) {
		filter [i] /= 16.0f;
	}
	CL::BufferRef filterBuffer = new CL::Buffer(context.get(), 9*sizeof(float), CL::ReadWrite_t::ReadOnly, CL::HostPtr_t::Copy, filter);

	CL::KernelRef kernel = new CL::Kernel(program.get(), "filter");
	CPPUNIT_ASSERT(kernel->setArgs(inImage, filterBuffer, outImage));

	//execute the kernel
	CPPUNIT_ASSERT(queue->execute(kernel.get(), {}, {256, 256}, {}));
	queue->finish();

	for(uint_fast32_t round = 0; round < 100; ++round) {
		rc.applyChanges();
		rc.clearScreen(ColorLibrary::BLACK);
		TextureUtils::drawTextureToScreen(rc, {0,0,256,256}, round < 50 ? *inTexture.get() : *outTexture.get(), {0,0,1,1});
		TestUtils::window->swapBuffers();
	}
}

void OpenCLTest::nativeKernelTest() {
	using namespace Util;
	using namespace Rendering;
	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;
	std::cout << "Native kernel support " << ((device->getExecutionCapabilities() & CL_EXEC_NATIVE_KERNEL) == CL_EXEC_NATIVE_KERNEL) << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get());
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());

	std::string testStr = "World";
	uint32_t answer = 42;

	// apparently lambda functions with reference capture does not work and results in a segmentation fault
//	CPPUNIT_ASSERT(queue->execute([&](){ answer = 42; }));
	CPPUNIT_ASSERT(queue->execute([](){ std::cout << "Hello "; }));

	queue->finish();

	std::cout << testStr << std::endl;
	CPPUNIT_ASSERT(answer == 42);
}

void OpenCLTest::bufferAccessorTest() {
	using namespace Util;
	using namespace Rendering;

	CL::PlatformRef platform;
	CL::DeviceRef device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_GPU);
	std::cout << std::endl << platform->getName() << std::endl << device->getName() << std::endl;
	std::cout << device->getOpenCL_CVersion() << std::endl;

	CL::ContextRef context = new CL::Context(platform.get(), device.get());
	CL::CommandQueueRef queue = new CL::CommandQueue(context.get(), device.get());

	char* outH = new char[hw.length()+1];
	outH[hw.length()] = 0; // end of string
	CL::BufferRef outCL = new CL::Buffer(context.get(), hw.length(), CL::ReadWrite_t::ReadWrite, CL::HostPtr_t::Use, outH);

	size_t size = outCL->getSize();
	CPPUNIT_ASSERT(size == hw.length());

	Reference<CL::BufferAccessor> acc = new CL::BufferAccessor(outCL, queue);
	acc->begin();
	CPPUNIT_ASSERT(static_cast<void*>(acc->_ptr()) == outH);
	for(uint_fast8_t i = 0; i < hw.length(); ++i) {
		acc->write(hw[i]);
		CPPUNIT_ASSERT(acc->getCursor() == static_cast<size_t>(i+1));
	}
	acc->end();

	CPPUNIT_ASSERT(queue->readBuffer(outCL.get(), true, 0, hw.length(), outH));
	queue->finish();

	CPPUNIT_ASSERT(hw.compare(outH) == 0);

	std::cout << outH;


	outCL = new CL::Buffer(context.get(), 100 * sizeof(int32_t), CL::ReadWrite_t::ReadWrite);

	CPPUNIT_ASSERT(outCL->getSize() == 100 * sizeof(int32_t));

	std::vector<int32_t> vec1(100);
	std::vector<int32_t> vec2;

	for(uint_fast32_t i=0; i<100; ++i)
		vec1[i] = i;

	acc = new CL::BufferAccessor(outCL, queue);
	CPPUNIT_ASSERT(!acc->isValid());
	acc->begin();
	CPPUNIT_ASSERT(acc->isValid());
	CPPUNIT_ASSERT(acc->getCursor() == 0);
	acc->writeArray(vec1);
	CPPUNIT_ASSERT(acc->getCursor() == 100 * sizeof(int32_t));
	acc->end();
	CPPUNIT_ASSERT(!acc->isValid());

	acc->begin();
	CPPUNIT_ASSERT(acc->getCursor() == 0);
	vec2.push_back(acc->read<int32_t>());
	vec2.push_back(acc->read<int32_t>());
	CPPUNIT_ASSERT(acc->getCursor() == 2 * sizeof(int32_t));
	auto tmp = acc->readArray<int32_t>(98);
	vec2.insert(vec2.end(), tmp.begin(), tmp.end());
	CPPUNIT_ASSERT(acc->getCursor() == 100 * sizeof(int32_t));
	acc->end();

	CPPUNIT_ASSERT(vec2.size() == 100);
	for(uint_fast32_t i=0; i<100; ++i)
		CPPUNIT_ASSERT(vec2[i] == static_cast<int32_t>(i));
}

#endif /* RENDERING_HAS_LIB_OPENCL */
