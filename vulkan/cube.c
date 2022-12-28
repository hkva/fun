#include "fun.h"

#include "vulkan/vulkan.h" // needs to be above SDL_vulkan

#include "SDL.h"
#include "SDL_vulkan.h"

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fun_err("Failed to initialize SDL: %s", SDL_GetError());
    }
    SDL_version ver; SDL_GetVersion(&ver);
    fun_msg("SDL version %d.%d.%d", ver.major, ver.minor, ver.patch);

    // Create window
    const Uint32 WNDFLAGS = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    SDL_Window* wnd = SDL_CreateWindow("Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, WNDFLAGS);
    FUN_ASSERT(wnd && "Failed to create SDL window");

    // Query extensions
    unsigned int ext_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(wnd, &ext_count, NULL)) {
        fun_err("Failed to get vulkan extensions: %s", SDL_GetError());
    }
    const char** ext = malloc(sizeof(char*) * ext_count); FUN_ASSERT(ext);
    if (!SDL_Vulkan_GetInstanceExtensions(wnd, &ext_count, ext)) {
        fun_err("Failed to get vulkan extensions: %s", SDL_GetError());
    }
    fun_msg("SDL_Vulkan_GetInstanceExtensions():");
    for (unsigned int i = 0; i < ext_count; ++i) {
        fun_msg("    %d: %s", i, ext[i]);
    }

    // Query validation layers
    const char* REQUESTED_VALIDATION_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    unsigned int vlr_count = 0;
    vkEnumerateInstanceLayerProperties(&vlr_count, NULL);
    VkLayerProperties* vlr = malloc(sizeof(VkLayerProperties) * vlr_count); FUN_ASSERT(vlr);
    vkEnumerateInstanceLayerProperties(&vlr_count, vlr);

    unsigned int evlr_count = 0;
    const char** evlr = NULL;

    fun_msg("Available validation layers:");
    for (unsigned int i = 0; i < vlr_count; ++i) {
        bool want = false;
        for (unsigned int j = 0; j < FUN_ARRLEN(REQUESTED_VALIDATION_LAYERS); ++j) {
            if ((want |= strcmp(vlr[i].layerName, REQUESTED_VALIDATION_LAYERS[j]) == 0)) {
                break;
            }
        }
        if (want) {
            evlr_count += 1;
            evlr = realloc(evlr, sizeof(char*) * evlr_count); FUN_ASSERT(evlr);
            evlr[evlr_count-1] = vlr[i].layerName;
        }

        fun_msg("    %d: %s (%s)", i, vlr[i].layerName, want ? "enabled" : "disabled");
    }

    // Create instance
    VkInstanceCreateInfo create_info = { 0 };
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount   = ext_count;
    create_info.ppEnabledExtensionNames = ext;
    create_info.enabledLayerCount       = evlr_count;
    create_info.ppEnabledLayerNames     = evlr;

    VkInstance instance = { 0 };
    if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
        fun_err("vkCreateInstance failed");
    }

    // Select physical device
    uint32_t dev_count;
    vkEnumeratePhysicalDevices(instance, &dev_count, NULL);
    if (dev_count == 0) {
        fun_err("No devices with Vulkan support detected");
    }
    VkPhysicalDevice* dev = malloc(sizeof(VkPhysicalDevice) * dev_count); FUN_ASSERT(dev);
    vkEnumeratePhysicalDevices(instance, &dev_count, dev);
    fun_msg("Physical devices:");
    for (uint32_t i = 0; i < dev_count; ++i) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(dev[i], &device_properties);
        fun_msg("    %d: %s", i, device_properties.deviceName);
    }
    // Use the first one
    VkPhysicalDevice dev_best = dev[0];

    // Find queue family for graphics commands
    uint32_t qfam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev_best, &qfam_count, NULL);
    VkQueueFamilyProperties* qfam = malloc(sizeof(VkQueueFamilyProperties) * qfam_count); FUN_ASSERT(qfam);
    vkGetPhysicalDeviceQueueFamilyProperties(dev_best, &qfam_count, qfam);
    int qfam_best = -1;
    for (uint32_t i = 0; i < qfam_count; ++i) {
        if (qfam[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            qfam_best = i;
            break;
        }
    }
    if (qfam_best == -1) {
        fun_err("Couldn't find a queue family with VK_QUEUE_GRAPHICS_BIT");
    }

    // Create logical device queue
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = { 0 };
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = qfam_best;
    queue_info.pQueuePriorities = &queue_priority;
    queue_info.queueCount = 1;

    // Create logical device
    VkDeviceCreateInfo ldevice_info = { 0 };
    ldevice_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ldevice_info.pQueueCreateInfos = &queue_info;
    ldevice_info.queueCreateInfoCount = 1;

    VkDevice ldevice = { 0 };
    if (vkCreateDevice(dev_best, &ldevice_info, NULL, &ldevice) != VK_SUCCESS) {
        fun_err("Failed to create logical device");
    }

    // Get queue handle
    VkQueue graphics_queue = { 0 };
    vkGetDeviceQueue(ldevice, qfam_best, 0, &graphics_queue);

    // Create surface
    VkSurfaceKHR surface = 0;
    if (!SDL_Vulkan_CreateSurface(wnd, instance, &surface)) {
        fun_err("Failed to create Vulkan surface: %s", SDL_GetError());
    }
}
