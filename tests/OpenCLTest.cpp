/*
 * OpenCLTest.cpp
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL

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
#include <Rendering/CL/Memory/Buffer.h>
#include <Rendering/CL/Memory/BufferGL.h>
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
float rand_float(float mn, float mx)
{
    float r = random() / (float) RAND_MAX;
    return mn + (mx-mn)*r;
}

void OpenCLTest::test() {
	using namespace Rendering;
	CL::Platform platform;
	CL::Device device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform.getName() << std::endl << device.getName() << std::endl;
	std::cout << device.getOpenCL_CVersion() << std::endl;

	CL::Context context(&platform, &device);
	CL::CommandQueue queue(&context, &device);
	CL::Program program(&context, {hw_kernel});
	CPPUNIT_ASSERT(program.build({device}));

	char* outH = new char[hw.length()-1];
	CL::Buffer outCL(&context, hw.length()-1, CL::Buffer::WriteOnly, CL::Buffer::Use, outH);

	CL::Kernel kernel(&program, "hello");
	CPPUNIT_ASSERT(kernel.setArg(0, &outCL));

	CPPUNIT_ASSERT(queue.execute(&kernel, {0}, {hw.length()+1}, {1}, {}));
	queue.finish();
	CPPUNIT_ASSERT(queue.readBuffer(&outCL, true, 0, hw.length()-1, outH));
	queue.finish();

	std::cout << outH;
}

void OpenCLTest::nativeKernelTest() {
	using namespace Rendering;
	using namespace Rendering;
	CL::Platform platform;
	CL::Device device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform.getName() << std::endl << device.getName() << std::endl;
	std::cout << device.getOpenCL_CVersion() << std::endl;

	CL::Context context(&platform, &device);
	CL::CommandQueue queue(&context, &device);
	CL::Program program(&context, {hw_kernel});
	CPPUNIT_ASSERT(program.build({device}));

}

void OpenCLTest::interopTest() {
	using namespace Rendering;
	using namespace Geometry;
	using namespace Util;

	CL::Platform platform;
	CL::Device device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_GPU);
	std::cout << std::endl << platform.getName() << std::endl << device.getName() << std::endl;

	CL::Context context(&platform, &device, true);
	CL::CommandQueue queue(&context, &device, false, true);
	CL::Program program(&context, {particle_kernel});
	CPPUNIT_ASSERT(program.build({device}));

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
	CL::BufferGL cl_vbo(&context, CL::Buffer::ReadWrite, mesh->_getVertexData()._getBufferId());

	//create the OpenCL only arrays
	CL::Buffer cl_velocities(&context, array_size, CL::Buffer::WriteOnly);
	CL::Buffer cl_pos_gen(&context, array_size, CL::Buffer::WriteOnly);
	CL::Buffer cl_vel_gen(&context, array_size, CL::Buffer::WriteOnly);

	//push our CPU arrays to the GPU
	//data is tightly packed in std::vector starting with the adress of the first element
    CPPUNIT_ASSERT(queue.writeBuffer(&cl_velocities, true, 0, array_size, &vel[0]));
    CPPUNIT_ASSERT(queue.writeBuffer(&cl_pos_gen, true, 0, array_size, &posGen[0]));
    CPPUNIT_ASSERT(queue.writeBuffer(&cl_vel_gen, true, 0, array_size, &vel[0]));
    queue.finish();

    //initialize our kernel from the program
	CL::Kernel kernel(&program, "part2");
    CPPUNIT_ASSERT(kernel.setArg(0, &cl_vbo)); //position vbo
    CPPUNIT_ASSERT(kernel.setArg(1, &cl_velocities));
    CPPUNIT_ASSERT(kernel.setArg(2, &cl_pos_gen));
    CPPUNIT_ASSERT(kernel.setArg(3, &cl_vel_gen));

    //Wait for the command queue to finish these commands before proceeding
    queue.finish();

    rc.setViewport({0,0,256,256});
    rc.setMatrix_modelToCamera(Matrix4x4f::orthographicProjection(-1,1,-1,1,-100,100));

    rc.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::NEVER));
    rc.pushAndSetLighting(LightingParameters(false));
    rc.pushAndSetBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
    rc.pushAndSetPointParameters(PointParameters(2, true));

    double time = 0;
//    CL::Event event;
	for(uint_fast32_t round = 0; round < 100; ++round) {
		rc.applyChanges();

		rc.clearScreen(ColorLibrary::WHITE);

		rc.finish();
		 // map OpenGL buffer object for writing from OpenCL
		//this passes in the vector of VBO buffer objects (position and color)
		queue.acquireGLObjects({&cl_vbo});
		queue.finish();

//	    CL::UserEvent userevent(&context);

		float dt = .01f;
		kernel.setArg(4, dt); //pass in the timestep
		//execute the kernel
		queue.execute(&kernel, {}, {num}, {});
//		event.setCallback(0, [&](const CL::Event& e, int32_t s){ std::cout << round << std::endl;});
//		userevent.setStatus(CL_COMPLETE);
		queue.finish();

//		time += (event.getProfilingCommandEnd() - event.getProfilingCommandStart()) * 1.0e-6;

		//Release the VBOs so OpenGL can play with them
		queue.releaseGLObjects({&cl_vbo});
		queue.finish();

		rc.displayMesh(mesh.get());

		TestUtils::window->swapBuffers();
	}
	std::cout << "Time: " << time << " ms (Avg: " << (time/1000) << " ms)"<< std::endl;
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
	CL::Platform platform;
	CL::Device device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_GPU);
	std::cout << std::endl << platform.getName() << std::endl << device.getName() << std::endl;
	std::cout << device.getOpenCL_CVersion() << std::endl;

	CL::Context context(&platform, &device, true);
	CPPUNIT_ASSERT((*context._internal())() != nullptr);
	CL::CommandQueue queue(&context, &device, false, false);
	CL::Program program(&context, {simple_filter});
	CPPUNIT_ASSERT(program.build({device}, "-D FILTER_SIZE=1"));

	Reference<Texture> inTexture = TextureUtils::createChessTexture(256, 256, 32);
	Reference<Texture> outTexture = TextureUtils::createChessTexture(256, 256, 32);

	// Textures needs to be on the GPU for further processing
	inTexture->_prepareForBinding(rc);
	outTexture->_prepareForBinding(rc);

	//make sure OpenGL is finished before we proceed
	rc.applyChanges();
	rc.finish();

	CL::Image inImage(&context, CL::Image::ReadWrite, inTexture.get());
	CL::Image outImage(&context, CL::Image::WriteOnly, outTexture.get());

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
	CL::Buffer filterBuffer(&context, 9*sizeof(float), CL::Memory::ReadOnly, CL::Memory::Copy, filter);

	CL::Kernel kernel(&program, "filter");
	CPPUNIT_ASSERT(kernel.setArg(0, &inImage));
	CPPUNIT_ASSERT(kernel.setArg(1, &filterBuffer));
	CPPUNIT_ASSERT(kernel.setArg(2, &outImage));

	CPPUNIT_ASSERT(queue.acquireGLObjects({&inImage, &outImage}));
	queue.finish();
	//execute the kernel
	CPPUNIT_ASSERT(queue.execute(&kernel, {}, {256, 256}, {}));
	queue.finish();
	CPPUNIT_ASSERT(queue.releaseGLObjects({&inImage, &outImage}));
	queue.finish();

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
	CL::Platform platform;
	CL::Device device;
	std::tie(platform, device) = CL::getFirstPlatformAndDeviceFor(CL::Device::TYPE_CPU);
	std::cout << std::endl << platform.getName() << std::endl << device.getName() << std::endl;
	std::cout << device.getOpenCL_CVersion() << std::endl;

	CL::Context context(&platform, &device, false);
	CPPUNIT_ASSERT((*context._internal())() != nullptr);
	CL::CommandQueue queue(&context, &device, false, false);
	CL::Program program(&context, {simple_filter});
	CPPUNIT_ASSERT(program.build({device}, "-D FILTER_SIZE=1"));

	Reference<Texture> inTexture = TextureUtils::createChessTexture(256, 256, 32);
	Reference<Texture> outTexture = TextureUtils::createChessTexture(256, 256, 32);

	Reference<Bitmap> inBitmap = inTexture->getLocalBitmap();
	Reference<Bitmap> outBitmap = outTexture->getLocalBitmap();
	std::fill(outBitmap->data(), outBitmap->data() + outBitmap->getDataSize(), 0);

	CL::Image inImage(&context, CL::Image::ReadOnly, inBitmap.get());
	CL::Image outImage(&context, CL::Image::ReadWrite, outBitmap.get());

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
	CL::Buffer filterBuffer(&context, 9*sizeof(float), CL::Memory::ReadOnly, CL::Memory::Copy, filter);

	CL::Kernel kernel(&program, "filter");
	CPPUNIT_ASSERT(kernel.setArg(0, &inImage));
	CPPUNIT_ASSERT(kernel.setArg(1, &filterBuffer));
	CPPUNIT_ASSERT(kernel.setArg(2, &outImage));

	//execute the kernel
	CPPUNIT_ASSERT(queue.execute(&kernel, {}, {256, 256}, {}));
	queue.finish();

//	CPPUNIT_ASSERT(queue.readImage(&outImage, {}, {256,256},0,0,outBitmap->data()));
//	queue.finish();


	for(uint_fast32_t round = 0; round < 100; ++round) {
		rc.applyChanges();
		rc.clearScreen(ColorLibrary::BLACK);
		TextureUtils::drawTextureToScreen(rc, {0,0,256,256}, round < 50 ? *inTexture.get() : *outTexture.get(), {0,0,1,1});
		TestUtils::window->swapBuffers();
	}
}

#endif /* RENDERING_HAS_LIB_OPENCL */
