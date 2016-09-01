/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_COMMANDQUEUE_H_
#define RENDERING_CL_COMMANDQUEUE_H_

#include "CLUtils.h"

#include <Util/ReferenceCounter.h>

#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <tuple>

namespace cl {
class CommandQueue;
}

namespace Rendering {
namespace CL {
class Device;
class Context;
class Buffer;
class Image;
class Memory;
class Kernel;
class Event;

class RangeND_t {
public:
	COMPILER_WARN_PUSH
	COMPILER_WARN_OFF(-Wmissing-field-initializers) // we don't care about uninitialized values above dim
	template<typename... Args>
	RangeND_t(Args&&... args) : dim(sizeof...(Args)), range({static_cast<size_t>(args)...}) {}
	COMPILER_WARN_POP
	size_t dim;
	std::array<size_t, 3> range;
};

struct MappedImage {
	void* ptr; //! A pointer to the image data
	size_t rowPitch; //! The row pitch of the mapped region
	size_t slicePitch; //! The slice pitch of the mapped region
};

class CommandQueue : public Util::ReferenceCounter<CommandQueue> {
public:
	typedef std::vector<Event*> EventList_t;

	/**
	 * Create a command-queue on a specific device.
	 *
	 * @param context Must be a valid OpenCL context.
	 * @param device Must be a device associated with context.
	 * @param outOfOrderExec Determines whether the commands queued in the command-queue are executed in-order or out-of-order.
	 * @param profiling Enable or disable profiling of commands in the command-queue.
	 */
	CommandQueue(Context* context, Device* device, bool outOfOrderExec = false, bool profiling = false);
	~CommandQueue();
	CommandQueue(const CommandQueue& queue);

	/**
	 * Enqueue commands to read from a buffer object to host memory.
	 *
	 * @param buffer Refers to a valid buffer object.
	 * @param blocking Indicates if the read operations are blocking or non-blocking.
	 * @param offset The offset in bytes in the buffer object to read from.
	 * @param size The size in bytes of data being read.
	 * @param ptr The pointer to buffer in host memory where data is to be read into.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool readBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueue commands to write to a buffer object from host memory.
	 *
	 * @param buffer Refers to a valid buffer object.
	 * @param blocking Indicates if the write operations are blocking or nonblocking.
	 * @param offset The offset in bytes in the buffer object to write to.
	 * @param size The size in bytes of data being written.
	 * @param ptr The pointer to buffer in host memory where data is to be written from.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool writeBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, const void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to copy from one buffer object to another.
	 *
	 * @param src Refers to a valid buffer object to copy data from.
	 * @param dst Refers to a valid buffer object to copy data into.
	 * @param srcOffset The offset where to begin copying data from src.
	 * @param dstOffset The offset where to begin copying data into dst.
	 * @param size Refers to the size in bytes to copy.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool copyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	
	/**
	 * Enqueue commands to write to a buffer object from host memory.
	 *
	 * @param buffer Refers to a valid buffer object.
	 * @param offset The location in bytes of the region being filled in buffer and must be a multiple of patternSize.
	 * @param size The size in bytes of region being filled in buffer and must be a multiple of patternSize.
	 * @param pattern A pointer to the data pattern of size patternSize in bytes.
	 * @param patternSize The size in bytes of data being written.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool fillBuffer(Buffer* buffer, size_t offset, size_t size, const void* pattern, size_t patternSize, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueue commands to read from a rectangular region from a buffer object to host memory.
	 *
	 * @param buffer Refers to a valid buffer object.
	 * @param blocking Indicates if the read operations are blocking or non-blocking.
	 * @param bufferOffset The (x, y, z) offset in the memory region associated with buffer.
	 * @param hostOffset The (x, y, z) offset in the memory region pointed to by ptr.
	 * @param region The (width, height, depth) in bytes of the 2D or 3D rectangle being read or written.
	 * @param ptr The pointer to buffer in host memory where data is to be read into.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @param bufferRowPitch The length of each row in bytes to be used for the memory region associated with buffer.
	 * @param bufferSlicePitch The length of each 2D slice in bytes to be used for the memory region associated with buffer.
	 * @param hostRowPitch The length of each row in bytes to be used for the memory region pointed to by ptr.
	 * @param hostSlicePitch The length of each 2D slice in bytes to be used for the memory region pointed to by ptr.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool readBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);

	/**
	 * Enqueue commands to write a rectangular region to a buffer object from host memory.
	 *
	 * @param buffer Refers to a valid buffer object.
	 * @param blocking Indicates if the write operations are blocking or non-blocking.
	 * @param bufferOffset The (x, y, z) offset in the memory region associated with buffer.
	 * @param hostOffset The (x, y, z) offset in the memory region pointed to by ptr.
	 * @param region The (width, height, depth) in bytes of the 2D or 3D rectangle being read or written.
	 * @param ptr The pointer to buffer in host memory where data is to be written from.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @param bufferRowPitch The length of each row in bytes to be used for the memory region associated with buffer.
	 * @param bufferSlicePitch The length of each 2D slice in bytes to be used for the memory region associated with buffer.
	 * @param hostRowPitch The length of each row in bytes to be used for the memory region pointed to by ptr.
	 * @param hostSlicePitch The length of each 2D slice in bytes to be used for the memory region pointed to by ptr.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool writeBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);

	/**
	 *
	 * @param src Refers to a valid buffer object to copy data from.
	 * @param dst Refers to a valid buffer object to copy data into.
	 * @param srcOrigin The (x, y, z) offset in the memory region associated with src.
	 * @param dstOrigin The (x, y, z) offset in the memory region associated with dst.
	 * @param region The (width, height, depth) in bytes of the 2D or 3D rectangle being copied.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @param bufferRowPitch The length of each row in bytes to be used for the memory region associated with src.
	 * @param bufferSlicePitch he length of each 2D slice in bytes to be used for the memory region associated with src.
	 * @param hostRowPitch The length of each row in bytes to be used for the memory region associated with dst.
	 * @param hostSlicePitch The length of each 2D slice in bytes to be used for the memory region associated with dst.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool copyBufferRect(Buffer* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);
	
	/**
	 * Enqueues a command to read from a 2D or 3D image object to host memory.
	 *
	 * @param image Refers to a valid 2D or 3D image object.
	 * @param blocking Indicates if the read operations are blocking or non-blocking.
	 * @param origin Defines the (x, y, z) offset in pixels in the image from where to read.
	 * @param region Defines the (width, height, depth) in pixels of the 2D or 3D rectangle being read.
	 * @param ptr The pointer to a buffer in host memory where image data is to be read from.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @param rowPitch The length of each row in bytes.
	 * @param slicePitch Size in bytes of the 2D slice of the 3D region of a 3D image being read.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool readImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t rowPitch = 0, size_t slicePitch = 0);

	/**
	 * Enqueues a command to write to a 2D or 3D image object from host memory.
	 *
	 * @param image Refers to a valid 2D or 3D image object.
	 * @param blocking Indicates if the write operation is blocking or non-blocking.
	 * @param origin Defines the (x, y, z) offset in pixels in the image from where to write.
	 * @param region Defines the (width, height, depth) in pixels of the 2D or 3D rectangle being written.
	 * @param ptr The pointer to a buffer in host memory where image data is to be written to.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @param rowPitch The length of each row in bytes.
	 * @param slicePitch Size in bytes of the 2D slice of the 3D region of a 3D image being written.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool writeImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t rowPitch = 0, size_t slicePitch = 0);

	/**
	 * Enqueues a command to copy image objects.
	 *
	 * @param src Refers to a valid 2D or 3D image object to copy from.
	 * @param dst Refers to a valid 2D or 3D image object to copy into.
	 * @param srcOrigin Defines the starting (x, y, z) location in pixels in src from where to start the data copy.
	 * @param dstOrigin Defines the starting (x, y, z) location in pixels in dst from where to start the data copy.
	 * @param region Defines the (width, height, depth) in pixels of the 2D or 3D rectangle to copy.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool copyImage(Image* src, Image* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to copy an image object to a buffer object.
	 *
	 * @param src A valid image object.
	 * @param dst A valid buffer object.
	 * @param srcOrigin Defines the (x, y, z) offset in pixels in the image from where to copy.
	 * @param region Defines the (width, height, depth) in pixels of the 2D or 3D rectangle to copy.
	 * @param dstOffset The offset where to begin copying data into dst.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool copyImageToBuffer(Image* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& region, size_t dstOffset, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to copy a buffer object to an image object.
	 *
	 * @param src A valid buffer object.
	 * @param dst A valid image object.
	 * @param srcOffset The offset where to begin copying data from src.
	 * @param dstOrigin The (x, y, z) offset in pixels where to begin copying data to dst.
	 * @param region Defines the (width, height, depth) in pixels of the 2D or 3D rectangle to copy.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool copyBufferToImage(Buffer* src, Image* dst, size_t srcOffset, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to map a region of the buffer object given by buffer into the host address space and returns a pointer to this mapped region.
	 *
	 * @param buffer A valid buffer object.
	 * @param blocking Indicates if the map operation is blocking or non-blocking.
	 * @param readWrite Indicates if the specified region in the buffer is being mapped for reading or writing.
	 * @param offset The offset in bytes of the region in the buffer object that is being mapped.
	 * @param size The size in bytes of the region in the buffer object that is being mapped.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return A pointer to the mapped region.
	 */
	void* mapBuffer(Buffer* buffer, bool blocking, ReadWrite_t readWrite, size_t offset, size_t size, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to map a region of an image object into the host address space and returns a pointer to this mapped region.
	 *
	 * @param image A valid image object.
	 * @param blocking Indicates if the map operation is blocking or non-blocking.
	 * @param readWrite Indicates if the specified region in the image is being mapped for reading or writing.
	 * @param origin Define the (x, y, z) offset in pixels of the 2D or 3D rectangle region that is to be mapped.
	 * @param region Define the (width, height, depth) in pixels of the 2D or 3D rectangle region that is to be mapped.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return An MappedImage struct containing the mapped pointer and the row-pitch and slice-pitch of the mapped region.
	 */
	MappedImage mapImage(Image* image, bool blocking, ReadWrite_t readWrite, const RangeND_t& origin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to unmap a previously mapped region of a memory object.
	 *
	 * @param memory A valid memory object.
	 * @param mappedPtr The host address returned by a previous call to mapBuffer or mapImage for memory.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool unmapMemory(Memory* memory, void* mappedPtr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to execute a kernel on a device.
	 *
	 * @param kernel A valid kernel object.
	 * @param offset Offset can be used to specify values (x,y,z) that describe the offset used to calculate the global ID of a work-item. Default is (0,0,0).
	 * @param global Values that describe the number of global work-items in specified dimensions (x,y,z) that will execute the kernel function.
	 * @param local Values that describe the number of work-items that make up a work-group (also referred to as the size of the work-group) that will execute the kernel specified by kernel.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	/**
	 * Enqueues a command to execute a kernel on a device.
	 *
	 * \note The kernel is executed using a single work-item.
	 * This is equivalent to calling execute with offset {0}, global-range set to {1} and local-range set to {1}
	 *
	 * @param kernel A valid kernel object.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool execute(Kernel* kernel, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * Enqueues a command to execute a native C/C++ function not compiled using the OpenCL compiler.
	 *
	 * \warning Using lambda functions with by-reference capture do not work,
	 * because OpenCL makes a copy of the function and loosing the references (results in a segmentation fault).
	 *
	 * @param kernel A host-callable user function.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this particular kernel execution instance.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool execute(std::function<void()> kernel, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * \brief Acquire OpenCL memory objects that have been created from OpenGL objects.
	 *
	 * These objects need to be acquired before they can be used by any OpenCL commands queued to a command-queue.
	 * The OpenGL objects are acquired by the OpenCL context associated with this command-queue and can therefore
	 * be used by all command-queues associated with the OpenCL context.
	 *
	 * @param buffers A list of CL memory objects that correspond to GL objects.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool acquireGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * \brief Release OpenCL memory objects that have been created from OpenGL objects.
	 *
	 * These objects need to be released before they can be used by OpenGL.
	 * The OpenGL objects are released by the OpenCL context associated with the command-queue.
	 *
	 * @param buffers A list of CL memory objects that correspond to GL objects.
	 * @param waitForEvents Specify events that need to complete before this particular command can be executed.
	 * @param event Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete.
	 * @return true if the operation was successful, false otherwise.
	 */
	bool releaseGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	/**
	 * \brief Enqueues a marker command.
	 *
	 * Enqueues a marker command to command_queue. The marker command is not completed until all commands enqueued before it have completed.
	 * The marker command returns an event which can be waited on, i.e. this event can be waited on to ensure that all commands which have
	 * been queued before the market command have been completed.
	 *
	 * @param event Returns an event which can be waited on.
	 */
	void marker(Event* event = nullptr);

	/**
	 * Enqueues a wait for a specific event or a list of events to complete before any future commands queued in the command-queue are executed.
	 * @param waitForEvents Events specified in waitForEvents act as synchronization points.
	 */
	void waitForEvents(const EventList_t& waitForEvents);

	/**
	 * \brief A synchronization point that enqueues a barrier operation.
	 *
	 * A barrier is a synchronization point that ensures that all queued commands in the command-queue have finished execution
	 * before the next batch of commands can begin execution.
	 */
	void barrier();

	/**
	 * Blocks until all previously queued OpenCL commands in a command-queue are issued to the associated device and have completed.
	 */
	void finish();

	/**
	 * Issues all previously queued OpenCL commands in a command-queue to the device associated with the command-queue.
	 */
	void flush();

	/**
	 * Returns the context associated with this command-queue.
	 * @return the context associated with this command-queue.
	 */
	Context* getContext() const { return context.get(); }

	/**
	 * Returns the device associated with this command-queue.
	 * @return the device associated with this command-queue.
	 */
	Device* getDevice() const { return device.get(); }
private:
	std::unique_ptr<cl::CommandQueue> queue;
	ContextRef context;
	DeviceRef device;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_COMMANDQUEUE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */
