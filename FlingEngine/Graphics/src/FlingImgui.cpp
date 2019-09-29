#include "FlingImgui.h"

namespace Fling
{
    FlingImgui::FlingImgui()
	{
        ImGui::CreateContext();
	}

	FlingImgui::~FlingImgui()
	{
        ImGui::DestroyContext();
        m_vertexBuffer.Release();
        m_indexBuffer.Release();
		vkDestroyImage(m_LogicalDevice->GetVkDevice(), m_fontImage, nullptr);
        vkDestroyImageView(m_LogicalDevice->GetVkDevice(), m_fontImageView, nullptr);
        vkFreeMemory(m_LogicalDevice->GetVkDevice(), m_fontMemory, nullptr);
        vkDestroySampler(m_LogicalDevice->GetVkDevice(), m_sampler, nullptr);
        vkDestroyPipelineCache(m_LogicalDevice->GetVkDevice(), m_pipelineCache, nullptr);
        vkDestroyPipelineLayout(m_LogicalDevice->GetVkDevice(), m_pipelineLayout, nullptr);
        vkDestroyDescriptorPool(m_LogicalDevice->GetVkDevice(), m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_LogicalDevice->GetVkDevice(), m_descriptorSetLayout, nullptr);
	}

	void FlingImgui::Init(float width, float height)
	{
        // Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        //Dimensions
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(width, height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	void FlingImgui::InitResources(VkRenderPass renderPass, VkQueue copyQueue)
	{
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* fontData;
        int texWidth;
        int texHeight;
        VkDevice logicalDevice = m_LogicalDevice->GetVkDevice();
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

        Fling::GraphicsHelpers::CreateVkImage(
            texWidth, 
            texHeight, 
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_fontImage,
            m_fontMemory);

        m_fontImageView = Fling::GraphicsHelpers::CreateVkImageView(
            m_fontImage,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

		Fling::GraphicsHelpers::TransitionImageLayout(
			m_fontImage,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		Fling::GraphicsHelpers::CreateVkSampler(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			m_sampler);

		//Descriptor pool
		std::vector<VkDescriptorPoolSize> poolSizes = 
		{
			Fling::GraphicsHelpers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = Fling::GraphicsHelpers::DescriptorPoolCreateInfo(poolSizes, 2);

		if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not create descriptor pool for imgui");
		}

		//Descriptor set layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			Fling::GraphicsHelpers::DescriptorSetLayoutBindings(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = Fling::GraphicsHelpers::DescriptorSetLayoutCreateInfo(setLayoutBindings);
		if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not create descriptor set layout for imgui");
		}

		//Descriptor set 
		VkDescriptorSetAllocateInfo allocInfo = Fling::GraphicsHelpers::DescriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);
		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &m_descriptorSet) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not allocate descriptor sets for imgui");
		}

		VkDescriptorImageInfo fontDescriptor = Fling::GraphicsHelpers::DescriptorImageInfo(
			m_sampler,
			m_fontImageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		std::vector<VkWriteDescriptorSet> writeDescriptorSet = {
			Fling::GraphicsHelpers::WriteDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor),
		};
		
		vkUpdateDescriptorSets(logicalDevice, static_cast<UINT32>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, nullptr);
        
		//Pipeline cache 
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		if (vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not create pipeline cache for imgui");
		}


		//Pipeline layout
		//Push constants for UI rendering 
		VkPushConstantRange pushConstantRange = Fling::GraphicsHelpers::PushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = Fling::GraphicsHelpers::PiplineLayoutCreateInfo(&m_descriptorSetLayout, 1);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not create pipline layout for imgui");
		}

		//Setup graphics pipeline for UI rendering 
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			Fling::GraphicsHelpers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, false);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			Fling::GraphicsHelpers::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		//Enable Blending 
		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendState = 
            Fling::GraphicsHelpers::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState = 
            Fling::GraphicsHelpers::DepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo viewportState = 
            Fling::GraphicsHelpers::PipelineViewportStateCreateInfo(1, 1, 0);

        VkPipelineMultisampleStateCreateInfo multisampleState = 
            Fling::GraphicsHelpers::PipelineMultiSampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState = 
            Fling::GraphicsHelpers::PipelineDynamicStateCreateInfo(dynamicStateEnables);

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = 
            Fling::GraphicsHelpers::PipelineCreateInfo(m_pipelineLayout, renderPass);

        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

        // Vertex bindings an attributes based on imgui vertex definitions
        std::vector<VkVertexInputBindingDescription> vertexInputBindings =
        {
            Fling::GraphicsHelpers::VertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
        };

        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
        {
            Fling::GraphicsHelpers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	// Location 0: Position
			Fling::GraphicsHelpers::VertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	// Location 1: UV
			Fling::GraphicsHelpers::VertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),	// Location 0: Color
        };

		VkPipelineVertexInputStateCreateInfo vertexInputState = Fling::GraphicsHelpers::PiplineVertexInptStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		//TODO: LOAD SHADERS imgui/ui.vert.spv
		// imgui/ui.frag.spv

		if (vkCreateGraphicsPipelines(logicalDevice, m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipeLine) != VK_SUCCESS)
		{
			F_LOG_ERROR("Could not create graphics pipeline for imgui");
		}
	}

	void FlingImgui::NewFrame()
	{
	}

	void FlingImgui::UpdateBuffers()
	{
	}

	void FlingImgui::DrawFrame()
	{
	}
}