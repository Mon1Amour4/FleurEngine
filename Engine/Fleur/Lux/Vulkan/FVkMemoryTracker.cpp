#include "FVkMemoryTracker.h"

#include <Fleur/Log.h>

#include <cassert>
#include <iomanip>
#include <sstream>

#include "VkHelper.h"

namespace
{
constexpr size_t CategoryIndex(FVkAllocationCategory category)
{
    return static_cast<size_t>(category);
}

const char* CategoryName(FVkAllocationCategory category)
{
    switch (category)
    {
    case FVkAllocationCategory::Buffer:
        return "Buffer";
    case FVkAllocationCategory::Staging:
        return "Staging";
    case FVkAllocationCategory::Texture:
        return "Texture";
    case FVkAllocationCategory::RenderTarget:
        return "RenderTarget";
    }

    return "Unknown";
}
}  // namespace

void FVkMemoryTracker::Init(VkPhysicalDevice physicalDevice, VkDevice device)
{
    assert(physicalDevice != VK_NULL_HANDLE);
    assert(device != VK_NULL_HANDLE);

    m_PhysicalDevice = physicalDevice;
    m_Device = device;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
}

VkDeviceMemory FVkMemoryTracker::Allocate(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties, FVkAllocationCategory category)
{
    assert(m_Device != VK_NULL_HANDLE);
    assert(requirements.size > 0);
    assert(requirements.memoryTypeBits != 0);

    uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;
    for (uint32_t index = 0; index < m_MemoryProperties.memoryTypeCount; ++index)
    {
        const bool typeAllowed = (requirements.memoryTypeBits & (1u << index)) != 0;
        const bool propertiesMatch = (m_MemoryProperties.memoryTypes[index].propertyFlags & properties) == properties;
        if (typeAllowed && propertiesMatch)
        {
            memoryTypeIndex = index;
            break;
        }
    }

    if (memoryTypeIndex == VK_MAX_MEMORY_TYPES)
    {
        FL_CORE_ERROR("[Vulkan] No compatible memory type found for allocation of {} bytes", requirements.size);
        assert(false);
        return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory));

    const uint32_t heapIndex = m_MemoryProperties.memoryTypes[memoryTypeIndex].heapIndex;
    const size_t categoryIndex = CategoryIndex(category);
    m_Allocations.emplace(HandleKey(memory), AllocationInfo{requirements.size, memoryTypeIndex, heapIndex, category});
    m_TotalTrackedBytes += requirements.size;
    m_TrackedBytesByHeap[heapIndex] += requirements.size;
    ++m_TrackedAllocationCountByHeap[heapIndex];
    m_TrackedBytesByCategory[categoryIndex] += requirements.size;
    ++m_TrackedAllocationCountByCategory[categoryIndex];

    FL_CORE_INFO("[Vulkan Memory] Allocated {}: {:.2f} MiB, memoryType={}, heap={}, total={:.2f} MiB", CategoryName(category),
                 static_cast<double>(requirements.size) / (1024.0 * 1024.0), memoryTypeIndex, heapIndex,
                 static_cast<double>(m_TotalTrackedBytes) / (1024.0 * 1024.0));

    return memory;
}

void FVkMemoryTracker::Free(VkDeviceMemory memory)
{
    if (memory == VK_NULL_HANDLE)
        return;

    const auto iterator = m_Allocations.find(HandleKey(memory));
    if (iterator == m_Allocations.end())
    {
        FL_CORE_ERROR("[Vulkan] Attempted to free an unknown VkDeviceMemory handle");
        assert(false);
        return;
    }

    const AllocationInfo& info = iterator->second;
    const size_t categoryIndex = CategoryIndex(info.category);
    vkFreeMemory(m_Device, memory, nullptr);
    m_TotalTrackedBytes -= info.size;
    m_TrackedBytesByHeap[info.heapIndex] -= info.size;
    --m_TrackedAllocationCountByHeap[info.heapIndex];
    m_TrackedBytesByCategory[categoryIndex] -= info.size;
    --m_TrackedAllocationCountByCategory[categoryIndex];
    m_Allocations.erase(iterator);
}

void FVkMemoryTracker::PrintDiagnosis() const
{
    constexpr double kMiB = 1024.0 * 1024.0;
    std::ostringstream report;
    report << "[Vulkan Memory]\n"
           << "  " << std::left << std::setw(16) << "Category" << std::right << std::setw(10) << "Objects" << std::setw(14) << "Allocations" << std::setw(14)
           << "Memory (MiB)" << '\n'
           << "  " << std::string(54, '-') << '\n';

    for (size_t index = 0; index < m_TrackedBytesByCategory.size(); ++index)
    {
        const auto category = static_cast<FVkAllocationCategory>(index);
        report << "  " << std::left << std::setw(16) << CategoryName(category) << std::right << std::setw(10) << m_TrackedAllocationCountByCategory[index]
               << std::setw(14) << m_TrackedAllocationCountByCategory[index] << std::setw(14) << std::fixed << std::setprecision(2)
               << (static_cast<double>(m_TrackedBytesByCategory[index]) / kMiB) << '\n';
    }

    report << "  " << std::string(54, '-') << '\n'
           << "  " << std::left << std::setw(16) << "Total" << std::right << std::setw(10) << m_Allocations.size() << std::setw(14) << m_Allocations.size()
           << std::setw(14) << std::fixed << std::setprecision(2) << (static_cast<double>(m_TotalTrackedBytes) / kMiB) << "\n\n"
           << "  " << std::left << std::setw(8) << "Heap" << std::right << std::setw(16) << "Allocations" << std::setw(18) << "Tracked (MiB)" << std::setw(18)
           << "Capacity (MiB)" << '\n'
           << "  " << std::string(60, '-') << '\n';

    for (uint32_t heap = 0; heap < m_MemoryProperties.memoryHeapCount; ++heap)
    {
        report << "  " << std::left << std::setw(8) << heap << std::right << std::setw(16) << m_TrackedAllocationCountByHeap[heap] << std::setw(18)
               << std::fixed << std::setprecision(2) << (static_cast<double>(m_TrackedBytesByHeap[heap]) / kMiB) << std::setw(18)
               << (static_cast<double>(m_MemoryProperties.memoryHeaps[heap].size) / kMiB) << '\n';
    }

    FL_CORE_INFO("{}", report.str());
}

size_t FVkMemoryTracker::GetLiveAllocationCount() const
{
    return m_Allocations.size();
}

uint64_t FVkMemoryTracker::HandleKey(VkDeviceMemory memory)
{
    return reinterpret_cast<uint64_t>(memory);
}
