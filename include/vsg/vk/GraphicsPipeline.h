#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/vk/Pipeline.h>
#include <vsg/vk/PipelineLayout.h>
#include <vsg/vk/ShaderModule.h>
#include <vsg/vk/RenderPass.h>
#include <vsg/vk/PipelineLayout.h>
#include <vsg/vk/Command.h>

namespace vsg
{

    class GraphicsPipelineState : public Object
    {
    public:
        GraphicsPipelineState() {}

        virtual VkStructureType getType() const = 0;

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const = 0;

    protected:
        virtual ~GraphicsPipelineState() {}
    };

    using GraphicsPipelineStates = std::vector<ref_ptr<GraphicsPipelineState>>;


    class VSG_EXPORT GraphicsPipeline : public Pipeline
    {
    public:
        virtual void accept(Visitor& visitor) override { visitor.apply(*this); }

        using Result = vsg::Result<GraphicsPipeline, VkResult, VK_SUCCESS>;

        /** Crreate a GraphicsPipeline.*/
        static Result create(Device* device, RenderPass* renderPass, PipelineLayout* pipelineLayout, const GraphicsPipelineStates& pipelineStates, AllocationCallbacks* allocator=nullptr);

    protected:
        GraphicsPipeline(VkPipeline pipeline, Device* device, RenderPass* renderPass, PipelineLayout* pipelineLayout, const GraphicsPipelineStates& pipelineStates, AllocationCallbacks* allocator);

        virtual ~GraphicsPipeline();

        ref_ptr<RenderPass>     _renderPass;
        GraphicsPipelineStates  _pipelineStates;
    };


    class VSG_EXPORT ShaderStages : public GraphicsPipelineState
    {
    public:
        ShaderStages(const ShaderModules& shaderModules);

        virtual VkStructureType getType() const override { return VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; }

        void setShaderModules(const ShaderModules& shaderModules) { _shaderModules = shaderModules; update(); }
        const ShaderModules& getShaderModules() const { return _shaderModules; }

        void update();

        std::size_t size() const { return _stages.size(); }

        VkPipelineShaderStageCreateInfo* data() { return _stages.data(); }
        const VkPipelineShaderStageCreateInfo* data() const { return _stages.data(); }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~ShaderStages();

        using Stages = std::vector<VkPipelineShaderStageCreateInfo>;
        Stages          _stages;
        ShaderModules   _shaderModules;
    };


    class VSG_EXPORT VertexInputState : public GraphicsPipelineState, public VkPipelineVertexInputStateCreateInfo
    {
    public:
        using Bindings = std::vector<VkVertexInputBindingDescription>;
        using Attributes = std::vector<VkVertexInputAttributeDescription>;

        VertexInputState();
        VertexInputState(const Bindings& bindings, const Attributes& attributes);

        virtual VkStructureType getType() const override { return sType; }

        const Bindings& geBindings() { return _bindings; }

        const Attributes& getAttributes() const { return _attributes; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~VertexInputState();

        Bindings                                _bindings;
        Attributes                              _attributes;
    };


    class VSG_EXPORT InputAssemblyState : public GraphicsPipelineState, public VkPipelineInputAssemblyStateCreateInfo
    {
    public:
        InputAssemblyState();

        virtual VkStructureType getType() const override { return sType; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~InputAssemblyState();
    };


    class VSG_EXPORT ViewportState : public GraphicsPipelineState, public VkPipelineViewportStateCreateInfo
    {
    public:
        ViewportState(const VkExtent2D& extent);

        virtual VkStructureType getType() const override { return sType; }

        VkViewport& getViewport() { return _viewport; }
        VkRect2D& getScissor() { return _scissor; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~ViewportState();

        VkViewport                          _viewport;
        VkRect2D                            _scissor;
    };


    class VSG_EXPORT RasterizationState : public GraphicsPipelineState, public VkPipelineRasterizationStateCreateInfo
    {
    public:
        RasterizationState();

        virtual VkStructureType getType() const override { return sType; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~RasterizationState();
    };


    class VSG_EXPORT MultisampleState : public GraphicsPipelineState, public VkPipelineMultisampleStateCreateInfo
    {
    public:
        MultisampleState();

        virtual VkStructureType getType() const override { return sType; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~MultisampleState();
    };


    class VSG_EXPORT DepthStencilState : public GraphicsPipelineState, public VkPipelineDepthStencilStateCreateInfo
    {
    public:
        DepthStencilState();

        virtual VkStructureType getType() const override { return sType; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

    protected:
        virtual ~DepthStencilState();
    };


    class VSG_EXPORT ColorBlendState : public GraphicsPipelineState, public VkPipelineColorBlendStateCreateInfo
    {
    public:
        ColorBlendState();

        virtual VkStructureType getType() const override { return sType; }

        virtual void apply(VkGraphicsPipelineCreateInfo& pipelineInfo) const override;

        using ColorBlendAttachments = std::vector<VkPipelineColorBlendAttachmentState>;
        const ColorBlendAttachments& getColorBlendAttachments() const { return _colorBlendAttachments; }

        void update();

    protected:
        virtual ~ColorBlendState();

        ColorBlendAttachments _colorBlendAttachments;
    };




}