#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define LEN(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(a, b) ((a < b) ? b : a)
#define MIN(a, b) ((a < b) ? a : b)

#define PASTER(x, y) x ## _ ## y
#define EVALUATOR(x, y) PASTER(x, y)

#define TRY(func) do { int8_t res = (func); if(res != 0) return res; } while(0)
#define TRY_MSG(func, msg) do { int8_t res = (func); if(res != 0) { printf(msg "\n"); return res; } } while(0)
#define TRY_DEFER(func) do { int8_t res = (func); if(res != 0) goto cleanup; } while(0)
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
#define OPT_EQ(A, B) ((A.__hasValue && B.__hasValue && A.__value == B.__value) || (!A.__hasValue && !B.__hasValue))

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

#define arrayListInitLocal(TYPE, NAME) arrayList(TYPE) NAME = \
{ \
	.count = 0, \
	.capacity = ARRAY_LIST_INIT_CAPACITY, \
	.items = (TYPE*)malloc(ARRAY_LIST_INIT_CAPACITY * sizeof(TYPE)), \
}
#define arrayListInitGlobal(TYPE, NAME) \
do \
{ \
	(NAME).count = 0; \
	(NAME).capacity = 4; \
	(NAME).items = (TYPE*)malloc(ARRAY_LIST_INIT_CAPACITY * sizeof(TYPE)); \
} while(0)
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
#define arrayListWithSize(TYPE, NAME, SIZE) \
do \
{ \
	(NAME).items = malloc(SIZE * sizeof(TYPE)); \
	(NAME).count = SIZE; \
	(NAME).capacity = SIZE; \
} while(0)

DEFINE_OPTION(uint32_t);
arrayListDefine(VkDeviceQueueCreateInfo)
arrayListDefine(uint32_t)
arrayListDefineContains(uint32_t)
arrayListDefine(VkSurfaceFormatKHR)
arrayListDefine(VkPresentModeKHR)
arrayListDefine(VkImage)
arrayListDefine(VkImageView)
arrayListDefine(VkFramebuffer)
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* validationLayers[1] = {"VK_LAYER_KHRONOS_validation"};
const char* deviceExtensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const int MAX_FRAMES_IN_FLIGHT = 2;
arrayListDefine(VkCommandBuffer)
arrayListDefine(VkSemaphore)
arrayListDefine(VkFence)

GLFWwindow* window = nullptr;
VkInstance instance = nullptr;
VkPhysicalDevice physicalDevice;
VkDevice globalDevice;
VkQueue graphicsQueue;
VkSurfaceKHR surface;
VkQueue presentQueue;
VkSwapchainKHR swapChain;
arrayList(VkImage) swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
arrayList(VkImageView) swapChainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
arrayList(VkFramebuffer) swapChainFramebuffers;
VkCommandPool commandPool;
arrayList(VkCommandBuffer) commandBuffers;
arrayList(VkSemaphore) imageAvailableSemaphores;
arrayList(VkSemaphore) renderFinishedSemaphores;
arrayList(VkFence) inFlightFences;
uint32_t currentFrame = 0;

typedef struct
{
	OPT(uint32_t) graphicsFamily;
	OPT(uint32_t) presentFamily;
} QueueFamilyIndices;

typedef struct
{
	VkSurfaceCapabilitiesKHR capabilites;
	arrayList(VkSurfaceFormatKHR) formats;
	arrayList(VkPresentModeKHR) presentModes;
} SwapChainSupportDetails;

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const arrayList(VkSurfaceFormatKHR) availableFormats)
{
	for(size_t i = 0; i < availableFormats.count; i++)
	{
		if(availableFormats.items[i].format == VK_FORMAT_B8G8R8_SRGB &&
			availableFormats.items[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormats.items[i];
		}
	}
	return availableFormats.items[0];
}
VkPresentModeKHR chooseSwapPresentMode(const arrayList(VkPresentModeKHR) availablePresentModes)
{
	for(size_t i = 0; i < availablePresentModes.count; i++)
	{
		if(availablePresentModes.items[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentModes.items[i];
		}
	}
	// NOTE: This one is guaranteed to be supported
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities)
{
	if(capabilities.currentExtent.width != UINT32_MAX &&
		capabilities.currentExtent.height != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D actualExtent =
	{
		.width = MIN(MAX((uint32_t)width, capabilities.minImageExtent.width), capabilities.maxImageExtent.width),
		.height = MIN(MAX((uint32_t)height, capabilities.minImageExtent.height), capabilities.maxImageExtent.height)
	};
	return actualExtent;
}

// NOTE: The caller is responsible for freeing the formats
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details = {0};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilites);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if(formatCount != 0)
	{
		arrayListWithSize(VkSurfaceFormatKHR, details.formats, formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.items);
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if(presentModeCount != 0)
	{
		arrayListWithSize(VkPresentModeKHR, details.presentModes, presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.items);
	}
	return details;
}

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
				break;
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
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if(presentSupport)
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

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	VkExtensionProperties availableExtensions[extensionCount];
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);
	foreach(const char*, extension, deviceExtensions,
	{
		bool found = false;
		foreach(VkExtensionProperties, property, availableExtensions,
		{
			if(strcmp(extension, property.extensionName) == 0)
			{
				found = true;
				break;
			}
		})
		if(!found)
		{
			return false;
		}
	})
	return true;
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if(extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = swapChainSupport.formats.count != 0 && swapChainSupport.presentModes.count != 0;
		arrayListFree(swapChainSupport.presentModes);
		arrayListFree(swapChainSupport.formats);
	}
	return extensionsSupported && swapChainAdequate && queueFamilyIndicesIsComplete(indices);
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
	arrayListInitLocal(VkDeviceQueueCreateInfo, queueCreateInfos);

	arrayListInitLocal(uint32_t, uniqueQueueFamilies);
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
		.enabledExtensionCount = LEN(deviceExtensions),
		.ppEnabledExtensionNames = deviceExtensions,
		.enabledLayerCount = LEN(validationLayers),
		.ppEnabledLayerNames = validationLayers
	};
	ASSERT_MSG_DEFER(vkCreateDevice(physicalDevice, &createInfo, nullptr, &globalDevice) == VK_SUCCESS, "Failed to create a logical device");
	vkGetDeviceQueue(globalDevice, OPT_VAL(indices.graphicsFamily), 0, &graphicsQueue);
	vkGetDeviceQueue(globalDevice, OPT_VAL(indices.presentFamily), 0, &presentQueue);
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

int8_t createSwapChain(void)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilites);
	uint32_t imageCount = swapChainSupport.capabilites.minImageCount + 1;
	if(swapChainSupport.capabilites.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilites.maxImageCount)
	{
		imageCount = swapChainSupport.capabilites.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = swapChainSupport.capabilites.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {OPT_VAL(indices.graphicsFamily), OPT_VAL(indices.presentFamily)};
	if(OPT_EQ(indices.graphicsFamily, indices.presentFamily))
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	int ret = 0;
	ASSERT_MSG_DEFER(vkCreateSwapchainKHR(globalDevice, &createInfo, nullptr, &swapChain) == VK_SUCCESS, "Failed to create a swap chain");
	vkGetSwapchainImagesKHR(globalDevice, swapChain, &imageCount, nullptr);
	arrayListWithSize(VkImage, swapChainImages, imageCount);
	vkGetSwapchainImagesKHR(globalDevice, swapChain, &imageCount, swapChainImages.items);
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
cleanup:
	arrayListFree(swapChainSupport.formats);
	arrayListFree(swapChainSupport.presentModes);
	return ret;
}

int8_t createImageViews(void)
{
	int ret = 0;
	arrayListWithSize(VkImageView, swapChainImageViews, swapChainImages.count);
	for(size_t i = 0; i < swapChainImages.count; i++)
	{
		VkImageViewCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages.items[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainImageFormat,
			.components =
			{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		ASSERT_MSG_DEFER(vkCreateImageView(globalDevice, &createInfo, nullptr, &swapChainImageViews.items[i]) == VK_SUCCESS, "Failed to create image views");
	}

cleanup:
	return ret;
}

// NOTE: The caller has to free the returned string
const char* readFile(const char* path, size_t* size)
{
	FILE* fp = fopen(path, "rb");
	if(!fp)
	{
		printf("Could not open\n");
		exit(1);
	}
	if(fseek(fp, 0, SEEK_END) != 0)
	{
		printf("Could not seek\n");
		exit(1);
	}
	int fSize = ftell(fp);
	if(fSize <= 0)
	{
		printf("Could not ftell\n");
		exit(1);
	}
	if(fseek(fp, 0, SEEK_SET) != 0)
	{
		printf("Could not seek\n");
		exit(1);
	}
	*size = fSize;
	char* contents = malloc(*size);
	if(fread(contents, sizeof(char), *size, fp) != *size)
	{
		printf("could not read\n");
		exit(1);
	}
	return contents;
}

VkShaderModule createShaderModule(const char* code, size_t size)
{
	VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (const uint32_t*)code,
	};
	VkShaderModule shaderModule;
	if(vkCreateShaderModule(globalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		printf("Failed to create shader module\n");
		exit(1);
	}
	return shaderModule;
}

int8_t createGraphicsPipeline(void)
{
	int8_t ret = 0;
	size_t vertSize;
	const char* vertShaderCode = readFile("./vert.spv", &vertSize);
	size_t fragSize;
	const char* fragShaderCode = readFile("./frag.spv", &fragSize);
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, vertSize);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, fragSize);
	VkPipelineShaderStageCreateInfo vertShaderStageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo fragShaderStageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main",
	};
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)swapChainExtent.width,
		.height = (float)swapChainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent = swapChainExtent,
	};
	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};
	VkPipelineRasterizationStateCreateInfo rasterizer =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};
	VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants =
		{
			0.0f,
			0.0f,
			0.0f,
			0.0f,
		},
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
	ASSERT_MSG_DEFER(vkCreatePipelineLayout(globalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS, "Failed to create pipeline layout");
	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = nullptr,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};
	ASSERT_MSG_DEFER(vkCreateGraphicsPipelines(globalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) == VK_SUCCESS, "Failed to create graphics pipeline");


cleanup:
	vkDestroyShaderModule(globalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(globalDevice, vertShaderModule, nullptr);
	free((void*)vertShaderCode);
	free((void*)fragShaderCode);
	return ret;
}

int8_t createRenderPass(void)
{
	int8_t ret = 0;
	VkAttachmentDescription colorAttachment =
	{
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkSubpassDescription subpass =
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
	};
	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};
	ASSERT_MSG(vkCreateRenderPass(globalDevice, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS, "Failed to create render pass");
	return ret;
}

int8_t createFramebuffers(void)
{
	int8_t ret = 0;
	for(size_t i = 0; i < swapChainImageViews.count; i++)
	{
		VkImageView attachments[] =
		{
			swapChainImageViews.items[i]
		};
		VkFramebufferCreateInfo framebufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1
		};
		VkFramebuffer framebuffer;
		ASSERT_MSG(vkCreateFramebuffer(globalDevice, &framebufferInfo, nullptr, &framebuffer) == VK_SUCCESS, "Failed to create framebuffer");
		arrayListAppend(VkFramebuffer, swapChainFramebuffers, framebuffer);
	}
	return ret;
}

int8_t createCommandPool(void)
{
	int8_t ret = 0;
	QueueFamilyIndices queuFamilyIndices = findQueueFamilies(physicalDevice);
	VkCommandPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = OPT_VAL(queuFamilyIndices.graphicsFamily),
	};
	ASSERT_MSG(vkCreateCommandPool(globalDevice, &poolInfo, nullptr, &commandPool) == VK_SUCCESS, "Failed to create a command pool");
	return ret;
}

int8_t createCommandBuffer(void)
{
	int8_t ret = 0;
	arrayListWithSize(VkCommandBuffer, commandBuffers, MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = commandBuffers.count
	};
	ASSERT_MSG(vkAllocateCommandBuffers(globalDevice, &allocInfo, commandBuffers.items) == VK_SUCCESS, "Failed to allocate command buffers");
	return ret;
}

int8_t recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	int8_t ret = 0;
	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = nullptr,
	};
	ASSERT_MSG(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin recording command buffer");
	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	VkRenderPassBeginInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = swapChainFramebuffers.items[imageIndex],
		.renderArea =
		{
			.offset =
			{
				.x = 0,
				.y = 0,
			},
			.extent = swapChainExtent,
		},
		.clearValueCount = 1,
		.pClearValues = &clearColor,
	};
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
	ASSERT_MSG(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Failed to record command buffer");
	return ret;
}

int8_t createSyncObjects(void)
{
	int8_t ret = 0;
	arrayListInitGlobal(VkSemaphore, imageAvailableSemaphores);
	arrayListInitGlobal(VkSemaphore, renderFinishedSemaphores);
	arrayListInitGlobal(VkFence, inFlightFences);
	VkSemaphoreCreateInfo semaphoreInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;
		VkFence inFlightFence;
		ASSERT_MSG(vkCreateSemaphore(globalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS &&
			vkCreateSemaphore(globalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) == VK_SUCCESS &&
			vkCreateFence(globalDevice, &fenceInfo, nullptr, &inFlightFence) == VK_SUCCESS, "Failed to create sync objects");
		arrayListAppend(VkSemaphore, imageAvailableSemaphores, imageAvailableSemaphore);
		arrayListAppend(VkSemaphore, renderFinishedSemaphores, renderFinishedSemaphore);
		arrayListAppend(VkFence, inFlightFences, inFlightFence);
	}
	return ret;
}

int8_t initVulkan(void)
{
	TRY(createInstance());
	TRY(createSurface());
	TRY(pickPhysicalDevice());
	TRY(createLogicalDevice());
	TRY(createSwapChain());
	TRY(createImageViews());
	TRY(createRenderPass());
	TRY(createGraphicsPipeline());
	TRY(createFramebuffers());
	TRY(createCommandPool());
	TRY(createCommandBuffer());
	TRY(createSyncObjects());
	return 0;
}

int8_t drawFrame()
{
	int8_t ret = 0;
	//printf("Frame %d, fence: %p\n", currentFrame, &inFlightFences.items[currentFrame]);
	ASSERT_MSG(vkWaitForFences(globalDevice, 1, &(inFlightFences.items[currentFrame]), VK_TRUE, UINT64_MAX) == VK_SUCCESS, "Could not wait for fences");
	vkResetFences(globalDevice, 1, &(inFlightFences.items[currentFrame]));
	uint32_t imageIndex;
	vkAcquireNextImageKHR(globalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores.items[currentFrame], VK_NULL_HANDLE, &imageIndex);
	vkResetCommandBuffer(commandBuffers.items[currentFrame], 0);
	recordCommandBuffer(commandBuffers.items[currentFrame], imageIndex);
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores.items[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores.items[currentFrame]};
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers.items[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};
	ASSERT_MSG(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences.items[currentFrame]) == VK_SUCCESS, "Failed to submit draw command buffer");
	VkSwapchainKHR swapChains[] = {swapChain};
	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,
	};
	ASSERT_MSG(vkQueuePresentKHR(presentQueue, &presentInfo) == VK_SUCCESS, "Failed to present queue");
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return ret;
}

int8_t mainLoop(void)
{
	int8_t ret = 0;
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		TRY_DEFER(drawFrame());
	}

cleanup:
	vkDeviceWaitIdle(globalDevice);
	return ret;
}

void cleanup(void)
{
	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(globalDevice, imageAvailableSemaphores.items[i], nullptr);
		vkDestroySemaphore(globalDevice, renderFinishedSemaphores.items[i], nullptr);
		vkDestroyFence(globalDevice, inFlightFences.items[i], nullptr);
	}
	vkDestroyCommandPool(globalDevice, commandPool, nullptr);
	for(size_t i = 0; i < swapChainFramebuffers.count; i++)
	{
		vkDestroyFramebuffer(globalDevice, swapChainFramebuffers.items[i], nullptr);
	}
	vkDestroyPipeline(globalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(globalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(globalDevice, renderPass, nullptr);
	for(size_t i = 0; i < swapChainImageViews.count; i++)
	{
		vkDestroyImageView(globalDevice, swapChainImageViews.items[i], nullptr);
	}
	vkDestroySwapchainKHR(globalDevice, swapChain, nullptr);
	vkDestroyDevice(globalDevice, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
	arrayListFree(swapChainImages);
	arrayListFree(swapChainImageViews);
	arrayListFree(swapChainFramebuffers);
	arrayListFree(imageAvailableSemaphores);
	arrayListFree(renderFinishedSemaphores);
	arrayListFree(inFlightFences);
}

uint8_t run(void)
{
	initWindow();
	TRY(initVulkan());
	TRY(mainLoop());
	cleanup();
	return 0;
}

int main(void)
{
	return run();
}
