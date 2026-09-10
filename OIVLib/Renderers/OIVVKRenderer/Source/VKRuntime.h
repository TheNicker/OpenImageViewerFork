#pragma once
#include <vulkan/vulkan.h>
#if defined(_WIN32) && !defined(OIV_VK_TEST_DOUBLES)
namespace OIV
{
    // Exported loader trampolines support independent instances/devices. Never replace these
    // with process-global pointers returned for a particular physical or logical device.
    #define OIV_VK_FUNCTION(name) extern PFN_##name name;
    #include "VKFunctions.inc"
    #undef OIV_VK_FUNCTION
    void EnsureVulkanRuntime();
}  // namespace OIV
#endif
