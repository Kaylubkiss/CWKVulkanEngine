
<h1 style="text-align: center;"> Multithreading In The (Untitled) Graphics Engine of Mine</h1>
<h3 style="text-align: center;">written by Caleb Kissinger</h3>

***Note (as of 4/13/26):** details of the implementation may change over time as the engine is still under development.*

## Introduction
Multithreading always feels like this daunting task that needs an incredible amount of skill to do.

The truth is, it doesn't take *that* much more understanding to get started on the specific task you want to accomplish.

However, it is a learning curve if you want the full breadth of the topic, and to do it right (which you should).

For my application, I decided to use a ***task queue*** as the backbone of asynchronous I/O operations, 
which in the engine primarily entails fetching and processing geometry and texture data, currently located in 
the "src/art/" directory of the project. 

|       ![texture directory.png](texture%20directory.png)        |
|:--------------------------------------------------------------:|
| The current "src/art/extern-texture" directory in the project. |


In the future, I may be interested in compiling shaders on a separate thread as well. 

## Threadpool Class

```c++
class ThreadPool 
{
public:
	ThreadPool() = default;
	~ThreadPool() = default;
	void Init(size_t num_threads);
	void Terminate();
	void EnqueueTask(std::function<void()>& task);
	bool isBusy();
private:
	void ThreadLoop();
private:
	std::mutex queue_mutex;
	std::condition_variable condition_variable;
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	bool terminate = false;
};
```

The idea is that we assign a separate set of threads on the CPU (specified by *num_threads*) to take in "tasks" from an 
underlying queue to be performed on that thread.

With any multithreaded implementation, one must ensure that each thread doesn't step on another thread's toes, often 
called a "data race" or "race condition."

In more technical terms, we must ensure that the work being performed on memory is exclusive to any given thread, or else:
*#crash#*

While there are several ways to achieve this (like *lock-free programming*), often coordination is done through 
***synchronization primitives.***

Assuming you need more of a refresher on concepts here:

 - A **condition variable** is a synchronization primitive that sleeps one or more threads until it's signaled by another 
thread and a specified ***predicate (condition)*** is satisfied.
 - **mutex** is short for "mutually exclusive." It's the prime synchronization primitive used to protect shared resources 
from being modified by multiple threads at once, and its atomic locking operations are used in combination 
with other primitives.

This is the last very important multithreading topic to cover for this implementation: **atomicity.**

None of the objects here themselves are atomic. The *locking* and *unlocking* of a mutex is. Atomicity essentially 
describes to observer threads that an operation either happened, or didn't happen. When an operation is performed, the
executing thread is not interrupted by others thanks to internal low-level instructions and memory barriers, 
making the result predictable, almost as if it were running on a single thread.

Pretty neat, right? Anyway, very important concept in multithreading.

### Queueing Up a Task
```c++
void ThreadPool::EnqueueTask(std::function<void()>& task)
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		tasks.push(std::move(task));
	}
	condition_variable.notify_one();
}
```
As you can see, the mutex is wrapped around a unique_lock, which is just a convenient and memory safe way to lock a 
mutex and ensure "tasks" is not edited by another thread until the caller thread of "tasks.push()" is finished. The 
condition variable is then signaled to wake up a thread waiting on its predicate to be true:

```c++
bool predicate = (!tasks.empty() || terminate);
```

The predicate here is asking if the queue is empty, or if the thread pool is currently being deallocated ("terminated").
``EnqueueTask()`` made ``!tasks.empty()`` true after pushing the task into the queue.

"Termination" takes a higher precedence over task execution, so if the thread pool is being destroyed, a queued
task may never be started.

### The Main Loop

```c++
void ThreadPool::ThreadLoop() 
{
	while (true) 
	{
		std::function<void()> func;

		{
		        //wait, wouldn't this lock everything forever?
			std::unique_lock<std::mutex> lock(queue_mutex);
		    
		        //calls unlock(), sleep(), then lock() after being signaled when the condition is true. 
			condition_variable.wait(lock, [this] {
				return (!tasks.empty() || terminate);
			});

			if (terminate)
			{
				return;
			}
            
                        //call std::move() to efficiently acquire the task. This turns tasks.front() from an 
                        //l-value to an r-value. operator=(std::function<void>&& other) is called, 
                        //swapping pointers rather than performing expensive copies.
                        //Note that tasks.front() becomes an empty (null) function pointer after this.
			func = std::move(tasks.front());
		    
		        //pop the task off the queue.
			tasks.pop();
		    
		}// queue_mutex.unlock()
        
	        //call the task (createObject(), createTexture(), etc.) on this thread.
		func();
	}
}
```

Now, the sharp student may ask: wouldn't ``queue_mutex`` lock all of the threads in perpetuity?

``condition_variable`` takes in a ``unique_lock``, which contains methods to manually ``unlock()`` and ``lock()`` 
a mutex on a given thread. When ``wait()`` is called, it will call ``unlock()`` and put the current 
thread to sleep, waiting for another thread to signal it at the time the queue is no longer empty or ``terminate`` is no 
longer false. Then, the condition variable will wake the thread, ``lock()`` the mutex or lock the thread until it can 
reacquire the mutex (perhaps it's being used by another thread), and then proceed with the loop logic.

The scope braces ensures the unique lock is released automatically by going out of scope after the closing brace ``}``, and 
the task (``func``) popped off the queue is then executed on the thread.

### Terminating the Thread Loop

```c++
void ThreadPool::Terminate()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        terminate = true;
    }

	condition_variable.notify_all();
	
	for (std::thread& active_thread : threads) 
	{
		active_thread.join();
	}
    
    threads.clear();
}
```

First, we ensure to acquire the queue mutex before setting the ``terminate`` flag to true. 
The condition variable then signals to wake up all the threads, they see that ``terminate`` is true, then leave
``ThreadPool::Loop()``. 

We then loop through all the currently running theads, calling ``.join()`` to ensure that their currently running task
is finished (returning from ``ThreadPool::Loop()``). The list of threads are then cleared and we return from the function.

## Syncing the Loaded Objects

In the engine, an **Object** simply represents a collection of meshes, textures, and physics parameters.

When creating an object, the user can specify its properties in a struct:

```c++
struct ObjectCreateInfo
{
	glm::mat4 modelTransform = glm::mat4(1.0f);
	PhysicsComponent physicsComponent;
	std::string objName;
	std::vector<std::string> textureFileNames;
	vk::Device* devicePtr = nullptr;
	TextureManager* textureManagerPtr = nullptr;
	bool hasPhysicsComponent = false;
};
```

This struct is 256 bytes, aligned along 4x64 byte cache lines. So, if these were processed along a shared array 
during parallelization, this alignment would prevent **false sharing**/performance slowdown (assuming 64 byte cache line). 
However, the ``ObjectCreateInfo`` struct is processed in an independent stack frame in the current implementation.

After passing in an ObjectCreateInfo variable, it's processed in an ``Object``, well, object. 
The object uses the information to load its resources, and upon completion is placed into an underlying map in an 
``AssetManager``.

```c++
void AssetManager::LoadObject( const ObjectCreateInfo& objectCI )
{
	std::function<void()> parallelFunction = [this, objectCI]()
	{
		{
			std::shared_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == true)
			{
				return;
			}
		}
        
	        //a bit of an aggressive enforcement -- maybe we only want to view geometry.
		assert(objectCI.textureManagerPtr != nullptr);

		//note: m_textureManager is also internally thread safe.
		auto newObject = std::make_unique<Object>(objectCI, *objectCI.textureManagerPtr);
		{
			std::unique_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == false)
			{
				m_objects[objectCI.objName] = std::move(newObject);
			}
		}
	};
    
        //Thread pool pushing the task (parallelFunction) into its underlying queue. 
	m_threadWorkers.EnqueueTask(parallelFunction);
}
```

Note that this is in no way a perfect way to load in objects; what if we want an object that has the same geometry but 
different texture, or model transformation? I've made the loading very simplistic so I could focus on other rendering systems. 
Eventually, though, I'll want to handle other cases to make the engine more production ready.

You may also notice that ``shared_lock`` object. This allows multiple threads to share a mutex lock. The mutex itself
is of type ``shared_mutex`` to keep an atomic counter of how many threads are using it and is required in order to use
a ``shared_locked``. 

Why would we want to do this? If the shared resource isn't being transformed in any way (like being read), 
then it's fine for multiple threads to read that resource without data corruption, allowing smoother parallelization. 
A ``shared_lock`` will not be able to acquire a mutex until it's been unlocked by any threads who have exclusive (write) 
access to a shared resource - assuming the programming is done right.

Also note that a ``shared_lock`` comes with a performance hit because of the atomic counting, and so a standard lock may 
be better in these instances if the encapsulated logic is quick. But, because I don't want any potential expansion 
of the parallel code to severely lock the main thread, I use this reader-writer pattern defensively. 
Maybe later I'll come back and change this.

### Syncing Texture Data

While geometry loading is rather uninteresting in the context of parallelization, as everything is done in one go,
textures were a bit more challenging due to the way Vulkan frees the underlying implementation to the programmer. 

I feel texture loading is quite exemplary of Vulkan's core components and low-level philosophy.

First, to load a texture, we grab its pixel data from disk. In the engine this is done with help of ``stb_image``, which is 
included as a submodule in the Github repo:

```c++
int textureWidth, textureHeight, textureChannels;

stbi_uc* pixels = stbi_load(filePath.c_str(),
    &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

if (pixels == nullptr)
{
    std::cerr << "could not load in specified texture " + filePath << std::endl;
    throw std::runtime_error("Texture() FAILED");
}
```

Then we must manually create a VkImage handle with backing GPU memory, which can be manipulated/accessed on the CPU 
through a vkDeviceMemory object, depending on the memory flag attributed to it during allocation. 
Because we want the texture to be efficiently accessed on the GPU, we use the ``VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT``
flag. This means we will have to rely on command buffers in order to manipulate the memory on the GPU.  

After creation, we must *transition* the image's layout, as it's initially ``VK_IMAGE_LAYOUT_UNDEFINED``. 
Most GPU drivers won't be able to interpret this without crashing or behaving badly. This is also part of why we must 
store the pixel data in a *staging buffer*, because transitioning the layout from ``UNDEFINED`` allows the driver to wipe any
pre-existing memory.


>The other reason for using a *staging buffer* is that we must be able to store the pixel data in 
``HOST_VISIBLE`` GPU memory, allowing us to write the pixel data to the GPU from the CPU. 

In Vulkan, we record image layout transitions with a ``VkImageMemoryBarrier``.

```c++
VkImageMemoryBarrier barrier = {};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.image = m_image;
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
barrier.srcAccessMask = 0;
barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
```

Because the image is not allocated with presentable (pixel) data, we first have to stage the image so that it can be
written to. This is done by transitioning its layout to ``VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`` and setting its access
mask to ``VK_ACCESS_TRANSFER_WRITE_BIT``.

We then recorded the layout transition specified by the `VkImageMemoryBarrier` using `vkCmdPipelineBarrier()`, 
and then  submit the recording to the GPU with `vkQueueSubmit()`.

```c++
//transition image to dst-optimal layout so the staging buffer can be copied into it.
{
    vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0, 0,
    nullptr, 0, nullptr, 1,
    &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

    VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCmd;

    {
        //prevent multiple threads from submitting to the same queue at once.
        std::lock_guard<std::mutex> lock(transferMutex);
        VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
            submissionFence));
    }

    VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
}
```

A few things to note:
1. the ``lock_guard``. This is to prevent multiple threads from submitting to the *transfer queue* at the same time, 
which is considered undefined in the Vulkan spec.
2. I am using a *dedicated* transfer queue to submit all layout transition commands. This is to avoid locking the main thread's 
graphics queue submissions every time I submit to the GPU.
3. ``submissionFence``. In Vulkan, you can, and should, use its number of synchronization primitives in order to properly,
well, synchronize, the commands submitted to the GPU - either between the GPU-CPU, or GPU-GPU. ``Fences`` are a GPU-CPU
primitive, and will lock the thread by calling ``vkWaitForFences()`` until the queue submission it was passed into is 
completed on the GPU. While I could use something else to allow the thread to move along, I'm ok with locking a worker 
thread for the convenience of the implementation, and to ensure correct behavior between queue submissions. 
I'll probably change this detail later. Note also that I call ``vkResetFences()`` to reset the fence's signaled bit or 
else subsequent calls to ``vkWaitForFences()`` will immediately move the thread along and potentially cause errors.

Now that the image's layout is GPU-defined (`TRANSFER_DST_OPTIMAL`), we can copy the pixel data from the staging buffer 
to the image. This is done with ``vkCmdCopyBufferToImage``.

```c++
//copy buffer into image.
{
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0,0,0 };
    region.imageExtent =
    {
        static_cast<uint32_t>(textureWidth),
        static_cast<uint32_t>(textureHeight),
        1
    };

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

    vkCmdCopyBufferToImage(transferCmd, stagingBuffer.GetHandle(), m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCmd;

    {
        std::lock_guard<std::mutex> lock(transferMutex);
        VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle,
            1, &submitInfo, submissionFence));
    }

    VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
}
```

Once the GPU has successfully completed the copy command, we can finally transition the image layout to a 
read-only format so that the texture image can be sampled in shaders.

Issue? Resources in the engine are *exclusive*, meaning only one queue family can own a resource. Because of the 
use of a dedicated transfer queue, we have to *release* this resource so that the graphics queue family can 
**acquire** it for rendering.

The process of releasing the texture resource is done with another ``VkImageMemoryBarrier``, but this time we set the 
``srcQueueFamilyIndex`` and ``dstQueueFamilyIndex``.

```c++
releaseBarrier.srcQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
releaseBarrier.dstQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::GRAPHICS).family;
```

We also specify the ``oldLayout`` and ``newLayout``.

```c++
releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
```

This ``vkCmdPipelineBarrier()`` *release* submission will then transition the layout as specified by the 
arguments and flush the local cache to the shared cache line (usually L2 on current hardware) 
so that the latest data is visible to other cores.

... Ok, so finally, the last piece of the handshake: **acquisition**. The graphics queue needs to contain and see 
the latest image data in order to read it.

In Vulkan, this is done with another ``vkCmdPipelineBarrier()`` submission. Per spec requirements, 
the ``oldLayout`` and ``newLayout`` parameters for this submission are exactly the same as the release barrier.

``dstAccessMask`` is set to ``VK_ACCESS_SHADER_READ_BIT`` so that the GPU knows to invalidate its cache before 
reading, and its ``dstStageMask`` is ``VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT`` so that the fragment shader doesn't
start before the operations done on the image (acquisition) is completed.

```c++
VkImageMemoryBarrier acquireBarrier = {};
acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
acquireBarrier.srcQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family;
acquireBarrier.dstQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family;
acquireBarrier.image = curr_texture->GetImage();
acquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
acquireBarrier.subresourceRange.baseMipLevel = 0;
acquireBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
acquireBarrier.subresourceRange.baseArrayLayer = 0;
acquireBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

acquireBarrier.srcAccessMask = 0;
acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

vkCmdPipelineBarrier(
    m_commandBuffers[currentFrame],
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //srcStageMask: since the transfer queue submission was waited on by the worker thread...
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, //dstStageMask
    0, 0, nullptr,
    0, nullptr,
    1, &acquireBarrier
);
```

>**NOTE:** I *should* synchronize the transfer and acquisition with a **semaphore** - a GPU-GPU primitive. Because
the release barrier is submitted on individual textures, I simply wait for the barrier submission to complete
with a fence on the worker thread to ease the complexity. Later on, I will need to create a system to 
batch a bunch of release barrier submissions which would processing multiple textures much faster for complex scenes.

I then submit this ``vkCmdPipelineBarrier()`` with a specialized ``textureUploadSemaphore`` so that the main thread doesn't need to
lock for the submission to complete. The GPU can simply wait on the semaphore to signal that it's done before proceeding
execution of subsequent ``vkQueueSubmit()`` calls.

```c++
bool textureSubmitted = m_textureManagerPtr->UploadTextureDataToGPU(currentFrame, textureUploadSemaphores[currentFrame]);
if (textureSubmitted == true)
{
    //wait for the work associated with the semaphore to complete before the fragment shader is ran.
    pipelineWaitStages.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    waitSemaphores.push_back(textureUploadSemaphores[currentFrame]);
}

submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
submitInfo.pWaitSemaphores = waitSemaphores.data();
submitInfo.pWaitDstStageMask = pipelineWaitStages.data();
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentImageIndex];
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &this->commandBuffers[currentFrame];

VK_CHECK_RESULT(vkQueueSubmit(device.GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
    inFlightFences[currentFrame]));
```

And... Tada. Async Object Loading.

![async_loading_01.gif](async_loading_01.gif)

## Conclusion

By using a task queue, blocking I/O now operates on separate threads and frees up the main thread to do other
tasks - in this case: compute physics, do rendering, updating currently loaded objects, etc. 

Multithreading usually manifests in more ways than just I/O though. Of course, a GPU is a parallelism monster, but even
physics, and creating render commands, can be done on a separate thread, freeing up the main thread even more to becoming
a coordinator instead of a performer.

This level of sophistication isn't required for my engine though, as the physics are very simple and I'm not doing extremely
steep multipass rendering setups. 

While I'm all about optimization, there is only so much time. The simulation itself
currently runs at a stable 144 fps on fully-powered machines, only dipping to 30 fps on much lower power. 




























