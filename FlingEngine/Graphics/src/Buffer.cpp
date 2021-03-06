#include "pch.h"
#include "Buffer.h"
#include "GraphicsHelpers.h"
#include "VulkanApp.h"	// #TODO Pass in the devices by arg and not using this singleton
#include "LogicalDevice.h"
#include "PhyscialDevice.h"

namespace Fling
{
    Buffer::Buffer(const VkDeviceSize& size, const VkBufferUsageFlags& t_Usage, const VkMemoryPropertyFlags& t_Properties, const void* t_Data)
		: m_Size(size)
		, m_Buffer(VK_NULL_HANDLE)
		, m_BufferMemory(VK_NULL_HANDLE)
	{
		CreateBuffer(m_Size, t_Usage, t_Properties, false, t_Data);

        m_Descriptor = {};
        m_Descriptor.buffer = m_Buffer;
        m_Descriptor.offset = m_Offset;
        m_Descriptor.range = m_Size;
	}

	Buffer::Buffer(const Buffer& t_Other)
	{
		if (*this != t_Other)
		{
			m_MappedMem = t_Other.m_MappedMem;
			m_Size = t_Other.m_Size;
			m_Buffer = t_Other.m_Buffer;
			m_BufferMemory = t_Other.m_BufferMemory;
			m_Descriptor = t_Other.m_Descriptor;
		}
	}

	bool Buffer::operator==(const Buffer& other) const
	{
		return m_Size == other.m_Size && m_Buffer == other.m_Buffer && m_MappedMem == other.m_MappedMem;
	}

	bool Buffer::operator!=(const Buffer& other) const
	{
		return !(*this == other);
	}

	VkResult Buffer::MapMemory(VkDeviceSize t_Size, VkDeviceSize t_Offset)
	{
		LogicalDevice* Dev = VulkanApp::Get().GetLogicalDevice();
		assert(Dev);
		VkDevice Device = Dev->GetVkDevice();
		return vkMapMemory(Device, m_BufferMemory, t_Offset, t_Size, 0, &m_MappedMem);
	}

	void Buffer::UnmapMemory()
	{
		if (m_MappedMem)
		{
			LogicalDevice* Dev = VulkanApp::Get().GetLogicalDevice();
			assert(Dev);
			VkDevice Device = Dev->GetVkDevice();			
			vkUnmapMemory(Device, m_BufferMemory);
			m_MappedMem = nullptr;
		}
	}

	void Buffer::CreateBuffer(
		const VkDeviceSize & t_size, 
		const VkBufferUsageFlags & t_Usage, 
		const VkMemoryPropertyFlags & t_Properties, 
		bool  t_unmapBuffer,
		const void * t_Data)
	{
		LogicalDevice* Dev = VulkanApp::Get().GetLogicalDevice();
		assert(Dev);
		VkDevice Device = Dev->GetVkDevice();

		PhysicalDevice* Phys = VulkanApp::Get().GetPhysicalDevice();
		assert(Phys);
		VkPhysicalDevice PhysicalDevice = Phys->GetVkPhysicalDevice();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = t_size;
		bufferInfo.usage = t_Usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(Device, &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
		{
			F_LOG_FATAL("Failed to create buffer!");
		}
		m_Size = t_size;
		//Get the memory requirements
		VkMemoryRequirements MemRequirments = {};
		vkGetBufferMemoryRequirements(Device, m_Buffer, &MemRequirments);

		VkMemoryAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		AllocInfo.allocationSize = MemRequirments.size;
		//Using VK_MEMORY_PROPERTY_HOST_COHERENT_BIT may cause worse perf,
		//we could use explicit flushing with vkFlushMappedMemoryRanges
		AllocInfo.memoryTypeIndex = GraphicsHelpers::FindMemoryType(PhysicalDevice, MemRequirments.memoryTypeBits, t_Properties);

		//Allocate the vertex buffer memory
		if (vkAllocateMemory(Device, &AllocInfo, nullptr, &m_BufferMemory) != VK_SUCCESS)
		{
			F_LOG_FATAL("Failed to alocate buffer memory!");
		}

		//Map this buffer and copy the data to the given data pointer if one was specified
		if (t_Data)
		{
			MapMemory();
			memcpy(m_MappedMem, t_Data, m_Size);

			if ((t_Properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange MappedRange{};
				MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				MappedRange.memory = m_BufferMemory;
				MappedRange.offset = 0;
				MappedRange.size = m_Size;
				vkFlushMappedMemoryRanges(Device, 1, &MappedRange);
			}

			if (t_unmapBuffer)
			{
				UnmapMemory();
			}
		}

		if ((vkBindBufferMemory(Device, m_Buffer, m_BufferMemory, 0) != VK_SUCCESS))
		{
			F_LOG_FATAL("Failed to bind buffer memory!");
		}
	}
	
	void Buffer::CopyBuffer(Buffer* t_SrcBuffer, Buffer* t_DstBuffer, VkDeviceSize t_Size)
	{
		assert(t_SrcBuffer && t_SrcBuffer->IsUsed() && t_DstBuffer && t_DstBuffer->IsUsed());

		// #TODO: Replace this graphics helper with a command buffer wrapper that handles the creation for us
		VkCommandBuffer commandBuffer = GraphicsHelpers::BeginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = t_Size;
		vkCmdCopyBuffer(commandBuffer, t_SrcBuffer->GetVkBuffer(), t_DstBuffer->GetVkBuffer(), 1, &copyRegion);

		GraphicsHelpers::EndSingleTimeCommands(commandBuffer);
	}

	void Buffer::Flush(VkDeviceSize t_size, VkDeviceSize t_offset)
	{
		LogicalDevice* Dev = VulkanApp::Get().GetLogicalDevice();
		assert(Dev);
		VkDevice LogicalDevice = Dev->GetVkDevice();

		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_BufferMemory;
		mappedRange.offset = t_offset;
		mappedRange.size = t_size;
		if (vkFlushMappedMemoryRanges(LogicalDevice, 1, &mappedRange) != VK_SUCCESS)
		{
			F_LOG_ERROR("Mapped buffer could not flush memory");
		}
	}

	void Buffer::Release()
	{
		// Free up the VK memory that this buffer uses
		UnmapMemory();
		
		LogicalDevice* Dev = VulkanApp::Get().GetLogicalDevice();
		assert(Dev);
		VkDevice Device = Dev->GetVkDevice();
		if (m_Buffer)
		{
			vkDestroyBuffer(Device, m_Buffer, nullptr);
			m_Buffer = nullptr;
		}

		if(m_BufferMemory)
		{
			vkFreeMemory(Device, m_BufferMemory, nullptr);
			m_BufferMemory = nullptr;
		}
	}

	Buffer::~Buffer()
	{
		Release();
	}
}   // namespace Fling