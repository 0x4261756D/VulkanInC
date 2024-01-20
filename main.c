#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define LEN(ARR) (sizeof(ARR)/sizeof(ARR[0]))

#define PASTER(x, y) x ## _ ## y
#define EVALUATOR(x, y) PASTER(x, y)

#define TRY(func) do { int8_t res = (func); if(res != 0) return res; } while(0)
#define TRY_MSG(func, msg) do { int8_t res = (func); if(res != 0) { printf(msg "\n"); return res; } } while(0)
#define ASSERT(condition) do { if(!(condition)) { return -1; } } while(0)
#define ASSERT_MSG(condition, msg) do { if(!(condition)) { printf(msg "\n"); return -1; } } while(0)
#define ASSERT_DEFER(condition) do { if(!(condition)) { return -1; } } while(0)
#define ASSERT_MSG_DEFER(condition, msg) do { if(!(condition)) { printf(msg "\n"); ret = -1; goto cleanup; } } while(0)

#define foreach(TYPE, ITEM, ITEMS, BODY) \
for(size_t EVALUATOR(i, __LINE__) = 0; EVALUATOR(i, __LINE__) < LEN(ITEMS); EVALUATOR(i, __LINE__)++) { TYPE ITEM = ITEMS[EVALUATOR(i, __LINE__)]; \
	BODY \
}

#define OPT(TYPE) EVALUATOR(TYPE, OPT)
#define OPT_ASSIGN(VAR, VAL) do { VAR.__hasValue = true; VAR.__value = (VAL); } while(0)
#define OPT_OK(VAR) (VAR.__hasValue)
#define OPT_VAL(VAR) (VAR.__value)
#define OPT_MUST(LS, VAR, RS) if(!VAR.__hasValue) return -1; LS (VAR) RS 

#define DEFINE_OPTION(TYPE) typedef struct \
{ \
	bool __hasValue; \
	TYPE __value; \
} OPT(TYPE)

#define ARRAY_LIST_INIT_CAPACITY 4
#define arrayList(TYPE) EVALUATOR(TYPE, AL)
#define arrayListDefine(TYPE) typedef struct\
{ \
	size_t count; \
	size_t capacity; \
	TYPE* items; \
} arrayList(TYPE);
#define arrayListDefineContains(TYPE) \
bool EVALUATOR(arrayListContains, arrayList(TYPE))(arrayList(TYPE) list, TYPE item) \
{ \
	for(size_t i = 0; i < list.count; i++) \
	{ \
		if(list.items[i] == item) \
		{ \
			return true; \
		} \
	} \
	return false; \
}
#define arrayListContains(TYPE, LIST, ITEM) EVALUATOR(arrayListContains, arrayList(TYPE))(LIST, ITEM)

#define arrayListInit(TYPE, NAME) arrayList(TYPE) NAME = \
{ \
	.count = 0, \
	.capacity = 4, \
	.items = (TYPE*)malloc(ARRAY_LIST_INIT_CAPACITY * sizeof(TYPE)), \
}
#define arrayListAppend(TYPE, NAME, ITEM) \
do \
{ \
	if((NAME).count >= (NAME).capacity) \
	{ \
		(NAME).capacity = ((NAME).capacity == 0) ? ARRAY_LIST_INIT_CAPACITY : ((NAME).capacity * 2); \
		(NAME).items = (TYPE*)realloc((NAME).items, (NAME.capacity) * sizeof(TYPE)); \
	} \
	(NAME).items[(NAME).count] = ITEM; \
	(NAME).count += 1; \
} while(0)
#define arrayListFree(NAME) \
do \
{ \
	free((NAME).items); \
	(NAME).count = 0; \
	(NAME).capacity = ARRAY_LIST_INIT_CAPACITY; \
} while(0)
#define arrayListRemoveAt(NAME, INDEX) \
do \
{ \
	ASSERT_MSG(INDEX < (NAME).count, "Attempted to access an index out of bounds"); \
	if(INDEX != (NAME).count - 1) \
	{ \
		memmove(&((NAME).items) + INDEX, &((NAME).items) + INDEX + 1, ((NAME).count - INDEX - 1) * sizeof((NAME).items)); \
	} \
	(NAME).count -= 1; \
} while(0)

DEFINE_OPTION(uint32_t);
arrayListDefine(VkDeviceQueueCreateInfo)
arrayListDefine(uint32_t)
arrayListDefineContains(uint32_t)
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* validationLayers[1] = {"VK_LAYER_KHRONOS_validation"};

GLFWwindow* window = nullptr;
VkInstance instance = nullptr;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue;
VkSurfaceKHR surface;
VkQueue presentQueue;

typedef struct
{
	OPT(uint32_t) graphicsFamily;
	OPT(uint32_t) presentFamily;
} QueueFamilyIndices;

inline bool queueFamilyIndicesIsComplete(QueueFamilyIndices indices)
{
	return OPT_OK(indices.graphicsFamily) && OPT_OK(indices.presentFamily);
}

void initWindow(void)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Hello, World", nullptr, nullptr);
}

bool checkLayerValidationSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	VkLayerProperties availableLayers[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	foreach(const char*, layerName, validationLayers,
	{
		bool layerFound = false;
		foreach(const VkLayerProperties, layerProperties, availableLayers,
		{
			if(strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
			}
		})
		if(!layerFound)
		{
			return false;
		}
	})
	return true;
}

int8_t createInstance(void)
{
	ASSERT_MSG(checkLayerValidationSupport(), "validation layers requested but not available");
	VkApplicationInfo appInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello, World",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	uint32_t glfwExtensionCount = 0;
	const char* const* glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions,
		.enabledLayerCount = LEN(validationLayers),
		.ppEnabledLayerNames = validationLayers,
	};
	ASSERT_MSG(vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS, "Could not create an instance");

	return 0;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = {0};
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
	for(uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			OPT_ASSIGN(indices.graphicsFamily, i);
		}
		VkBool32 presentSupport = false;
		if(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport))
		{
			OPT_ASSIGN(indices.presentFamily, i);
		}
		if(queueFamilyIndicesIsComplete(indices))
		{
			break;
		}
	}
	return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);
	return OPT_OK(indices.graphicsFamily);
}

int8_t pickPhysicalDevice(void)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	ASSERT_MSG(deviceCount != 0, "failed to find GPUs with Vulkan support");
	VkPhysicalDevice devices[deviceCount];
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
	foreach(const VkPhysicalDevice, device, devices,
	{
		if(isDeviceSuitable(device))
		{
			physicalDevice = device;
			break;
		}
	})
	ASSERT_MSG(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU");
	return 0;
}

int8_t createLogicalDevice()
{
	int8_t ret = 0;
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	arrayListInit(VkDeviceQueueCreateInfo, queueCreateInfos);

	arrayListInit(uint32_t, uniqueQueueFamilies);
	arrayListAppend(uint32_t, uniqueQueueFamilies, OPT_VAL(indices.graphicsFamily));
	uint32_t presentFamilyIndex = OPT_VAL(indices.presentFamily);
	if(!arrayListContains(uint32_t, uniqueQueueFamilies, presentFamilyIndex))
	{
		arrayListAppend(uint32_t, uniqueQueueFamilies, OPT_VAL(indices.presentFamily));
	}
	float queuePriority = 1.0f;
	for(size_t i = 0; i < uniqueQueueFamilies.count; i++)
	{
		VkDeviceQueueCreateInfo queueCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = uniqueQueueFamilies.items[i],
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};
		arrayListAppend(VkDeviceQueueCreateInfo, queueCreateInfos, queueCreateInfo);
	}
	VkPhysicalDeviceFeatures deviceFeatures = {0};
	VkDeviceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos = queueCreateInfos.items,
		.queueCreateInfoCount = queueCreateInfos.count,
		.pEnabledFeatures = &deviceFeatures,
		.enabledExtensionCount = 0,
		.enabledLayerCount = LEN(validationLayers),
		.ppEnabledLayerNames = validationLayers
	};
	ASSERT_MSG_DEFER(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) == VK_SUCCESS, "Failed to create a logical device");
	vkGetDeviceQueue(device, OPT_VAL(indices.graphicsFamily), 0, &graphicsQueue);
	vkGetDeviceQueue(device, OPT_VAL(indices.presentFamily), 0, &presentQueue);
cleanup:
	arrayListFree(uniqueQueueFamilies);
	arrayListFree(queueCreateInfos);
	return ret;
}

int8_t createSurface(void)
{
	ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface) == VK_SUCCESS);
	return 0;
}

int8_t initVulkan(void)
{
	TRY(createInstance());
	TRY(createSurface());
	TRY(pickPhysicalDevice());
	TRY(createLogicalDevice());
	return 0;
}

void mainLoop(void)
{
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void cleanup(void)
{
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

uint8_t run(void)
{
	initWindow();
	TRY(initVulkan());
	mainLoop();
	cleanup();
	return 0;
}



int main(void)
{
	return run();
}
