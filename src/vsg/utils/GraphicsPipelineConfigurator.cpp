/* <editor-fold desc="MIT License">

Copyright(c) 2022 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/app/View.h>
#include <vsg/io/Logger.h>
#include <vsg/io/Options.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/ViewDependentState.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/SharedObjects.h>
#include <vsg/vk/Context.h>

using namespace vsg;

struct AssignGraphicsPipelineStates : public vsg::Visitor
{
    vsg::GraphicsPipelineConfigurator* config = nullptr;

    explicit AssignGraphicsPipelineStates(vsg::GraphicsPipelineConfigurator* in_config) :
        config(in_config) {}

    ref_ptr<ColorBlendState> colorBlendState;
    ref_ptr<DepthStencilState> depthStencilState;
    ref_ptr<DynamicState> dynamicState;
    ref_ptr<InputAssemblyState> inputAssemblyState;
    ref_ptr<MultisampleState> multisampleState;
    ref_ptr<RasterizationState> rasterizationState;
    ref_ptr<TessellationState> tessellationState;
    ref_ptr<VertexInputState> vertexInputState;
    ref_ptr<ViewportState> viewportState;

    void apply(vsg::Object& object) override { object.traverse(*this); }
    void apply(vsg::ColorBlendState& cbs) override
    {
        colorBlendState = ColorBlendState::create(cbs);
        config->pipelineStates.push_back(colorBlendState);
    }
    void apply(vsg::DepthStencilState& mss) override
    {
        depthStencilState = DepthStencilState::create(mss);
        config->pipelineStates.push_back(depthStencilState);
    }
    void apply(vsg::DynamicState& mss) override
    {
        dynamicState = DynamicState::create(mss);
        config->pipelineStates.push_back(dynamicState);
    }
    void apply(vsg::InputAssemblyState& ias) override
    {
        inputAssemblyState = InputAssemblyState::create(ias);
        config->pipelineStates.push_back(inputAssemblyState);
    }
    void apply(vsg::MultisampleState& mss) override
    {
        multisampleState = MultisampleState::create(mss);
        config->pipelineStates.push_back(multisampleState);
    }
    void apply(vsg::RasterizationState& rs) override
    {
        rasterizationState = RasterizationState::create(rs);
        config->pipelineStates.push_back(rasterizationState);
    }
    void apply(vsg::TessellationState& rs) override
    {
        tessellationState = TessellationState::create(rs);
        config->pipelineStates.push_back(tessellationState);
    }
    void apply(vsg::VertexInputState& ias) override
    {
        vertexInputState = VertexInputState::create(ias);
        config->pipelineStates.push_back(vertexInputState);
    }
    void apply(vsg::ViewportState& ias) override
    {
        viewportState = ViewportState::create(ias);
        config->pipelineStates.push_back(viewportState);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DescriptorConfigurator
//
DescriptorConfigurator::DescriptorConfigurator(ref_ptr<ShaderSet> in_shaderSet) :
    shaderSet(in_shaderSet)
{
}

int DescriptorConfigurator::compare(const Object& rhs_object) const
{
    int result = Object::compare(rhs_object);
    if (result != 0) return result;

    auto& rhs = static_cast<decltype(*this)>(rhs_object);

    if ((result = compare_pointer(shaderSet, rhs.shaderSet))) return result;
    if ((result = compare_value(blending, rhs.blending))) return result;
    if ((result = compare_value(two_sided, rhs.two_sided))) return result;
    if ((result = compare_container(assigned, rhs.assigned))) return result;
    if ((result = compare_container(defines, rhs.defines))) return result;
    return compare_pointer_container(descriptorSets, rhs.descriptorSets);
}

void DescriptorConfigurator::reset()
{
    assigned.clear();
    defines.clear();
    descriptorSets.clear();
}

bool DescriptorConfigurator::enableTexture(const std::string& name)
{
    if (auto& textureBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!textureBinding.define.empty()) defines.insert(textureBinding.define);
        auto sampler = Sampler::create();

        // create texture image and associated DescriptorSets and binding
        return assignDescriptor(textureBinding.set, textureBinding.binding, textureBinding.descriptorType, textureBinding.descriptorCount, textureBinding.stageFlags,
                                DescriptorImage::create(sampler, textureBinding.data, textureBinding.binding, 0, textureBinding.descriptorType));
    }
    return false;
}

bool DescriptorConfigurator::assignTexture(const std::string& name, ref_ptr<Data> textureData, ref_ptr<Sampler> sampler, uint32_t dstArrayElement)
{
    if (auto& textureBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!textureBinding.define.empty()) defines.insert(textureBinding.define);
        if (!sampler) sampler = Sampler::create();

        // create texture image and associated DescriptorSets and binding
        return assignDescriptor(textureBinding.set, textureBinding.binding, textureBinding.descriptorType, textureBinding.descriptorCount, textureBinding.stageFlags,
                                DescriptorImage::create(sampler, textureData ? textureData : textureBinding.data, textureBinding.binding, dstArrayElement, textureBinding.descriptorType));
    }
    return false;
}

bool DescriptorConfigurator::assignTexture(const std::string& name, const ImageInfoList& imageInfoList, uint32_t dstArrayElement)
{
    if (auto& textureBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!textureBinding.define.empty()) defines.insert(textureBinding.define);

        // create texture image and associated DescriptorSets and binding
        return assignDescriptor(textureBinding.set, textureBinding.binding, textureBinding.descriptorType, textureBinding.descriptorCount, textureBinding.stageFlags,
                                DescriptorImage::create(imageInfoList, textureBinding.binding, dstArrayElement, textureBinding.descriptorType));
    }
    return false;
}

bool DescriptorConfigurator::enableDescriptor(const std::string& name)
{
    if (auto& descriptorBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!descriptorBinding.define.empty()) defines.insert(descriptorBinding.define);

        // create uniform and associated DescriptorSets and binding
        return assignDescriptor(descriptorBinding.set, descriptorBinding.binding, descriptorBinding.descriptorType, descriptorBinding.descriptorCount, descriptorBinding.stageFlags,
                                DescriptorBuffer::create(descriptorBinding.data, descriptorBinding.binding));
    }
    return false;
}

bool DescriptorConfigurator::assignDescriptor(const std::string& name, ref_ptr<Data> data, uint32_t dstArrayElement)
{
    if (auto& descriptorBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!descriptorBinding.define.empty()) defines.insert(descriptorBinding.define);

        // create uniform and associated DescriptorSets and binding
        return assignDescriptor(descriptorBinding.set, descriptorBinding.binding, descriptorBinding.descriptorType, descriptorBinding.descriptorCount, descriptorBinding.stageFlags,
                                DescriptorBuffer::create(data ? data : descriptorBinding.data, descriptorBinding.binding, dstArrayElement, descriptorBinding.descriptorType));
    }
    return false;
}

bool DescriptorConfigurator::assignDescriptor(const std::string& name, const BufferInfoList& bufferInfoList, uint32_t dstArrayElement)
{
    if (auto& descriptorBinding = shaderSet->getDescriptorBinding(name))
    {
        assigned.insert(name);

        // set up bindings
        if (!descriptorBinding.define.empty()) defines.insert(descriptorBinding.define);

        // create uniform and associated DescriptorSets and binding
        return assignDescriptor(descriptorBinding.set, descriptorBinding.binding, descriptorBinding.descriptorType, descriptorBinding.descriptorCount, descriptorBinding.stageFlags,
                                DescriptorBuffer::create(bufferInfoList, descriptorBinding.binding, dstArrayElement, descriptorBinding.descriptorType));
    }
    return false;
}

bool DescriptorConfigurator::assignDescriptor(uint32_t set, uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, ref_ptr<Descriptor> descriptor)
{
    if (auto currentSize = descriptorSets.size(); set >= currentSize)
    {
        descriptorSets.resize(set + 1);
    }

    auto& ds = descriptorSets[set];
    if (!ds)
    {
        ds = vsg::DescriptorSet::create();
        ds->setLayout = DescriptorSetLayout::create();
    }

    ds->descriptors.push_back(descriptor);

    auto& descriptorBindings = ds->setLayout->bindings;
    descriptorBindings.push_back(VkDescriptorSetLayoutBinding{binding, descriptorType, descriptorCount, stageFlags, nullptr});

    return true;
}

bool DescriptorConfigurator::assignDefaults(const std::set<uint32_t>& inheritedSets)
{
    bool assignedDefault = false;
    if (shaderSet)
    {
        for (auto& descriptorBinding : shaderSet->descriptorBindings)
        {
            if (inheritedSets.count(descriptorBinding.set) != 0)
            {
                info("DescriptorConfigurator::assignDefaults()(..) no need to assign on set ", descriptorBinding.set);
                continue;
            }

            if (descriptorBinding.define.empty() && assigned.count(descriptorBinding.name) == 0)
            {
                bool set_matched = false;
                for (auto& cds : shaderSet->customDescriptorSetBindings)
                {
                    if (cds->set == descriptorBinding.set)
                    {
                        set_matched = true;
                        break;
                    }
                }
                if (!set_matched && descriptorBinding.data)
                {
                    bool isTexture = false;
                    switch (descriptorBinding.descriptorType)
                    {
                    case (VK_DESCRIPTOR_TYPE_SAMPLER):
                    case (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER):
                    case (VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE):
                    case (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE):
                        isTexture = true;
                        break;
                    default:
                        break;
                    }

                    if (isTexture)
                    {
                        assignDescriptor(descriptorBinding.set, descriptorBinding.binding, descriptorBinding.descriptorType, descriptorBinding.descriptorCount, descriptorBinding.stageFlags,
                                         DescriptorImage::create(Sampler::create(), descriptorBinding.data, descriptorBinding.binding, 0, descriptorBinding.descriptorType));
                    }
                    else
                    {
                        assignDescriptor(descriptorBinding.set, descriptorBinding.binding, descriptorBinding.descriptorType, descriptorBinding.descriptorCount, descriptorBinding.stageFlags,
                                         DescriptorBuffer::create(descriptorBinding.data, descriptorBinding.binding));
                    }

                    assigned.insert(descriptorBinding.name);
                    assignedDefault = true;
                }
            }
        }
    }

    return assignedDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ArrayConfigurator
//
ArrayConfigurator::ArrayConfigurator(ref_ptr<ShaderSet> in_shaderSet) :
    shaderSet(in_shaderSet)
{
}

int ArrayConfigurator::compare(const Object& rhs_object) const
{
    int result = Object::compare(rhs_object);
    if (result != 0) return result;

    auto& rhs = static_cast<decltype(*this)>(rhs_object);

    if ((result = compare_pointer(shaderSet, rhs.shaderSet))) return result;
    if ((result = compare_value(baseAttributeBinding, rhs.baseAttributeBinding))) return result;
    if ((result = compare_container(assigned, rhs.assigned))) return result;
    if ((result = compare_container(defines, rhs.defines))) return result;
    if ((result = compare_value_container(vertexBindingDescriptions, rhs.vertexBindingDescriptions))) return result;
    return compare_value_container(vertexAttributeDescriptions, rhs.vertexAttributeDescriptions);
}

bool ArrayConfigurator::assignArray(const std::string& name, VkVertexInputRate vertexInputRate, ref_ptr<Data> array)
{
    auto& attributeBinding = shaderSet->getAttributeBinding(name);
    if (attributeBinding)
    {
        assigned.insert(name);

        VkFormat format = array ? array->properties.format : VK_FORMAT_UNDEFINED;

        // set up bindings
        uint32_t bindingIndex = baseAttributeBinding + static_cast<uint32_t>(arrays.size());
        if (!attributeBinding.define.empty()) defines.insert(attributeBinding.define);

        vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{attributeBinding.location, bindingIndex, (format != VK_FORMAT_UNDEFINED) ? format : attributeBinding.format, 0});
        vertexBindingDescriptions.push_back(VkVertexInputBindingDescription{bindingIndex, array->properties.stride, vertexInputRate});

        arrays.push_back(array);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GraphicsPipelineConfigurator
//
GraphicsPipelineConfigurator::GraphicsPipelineConfigurator(ref_ptr<ShaderSet> in_shaderSet) :
    shaderSet(in_shaderSet)
{
    _assignShaderSetSettings();
}

void GraphicsPipelineConfigurator::_assignShaderSetSettings()
{
    // apply defaults
    AssignGraphicsPipelineStates agps(this);
    for (auto& pipelineState : shaderSet->defaultGraphicsPipelineStates)
    {
        pipelineState->accept(agps);
    }

    if (!agps.vertexInputState) pipelineStates.push_back(VertexInputState::create());
    if (!agps.inputAssemblyState) pipelineStates.push_back(vsg::InputAssemblyState::create());
    if (!agps.rasterizationState) pipelineStates.push_back(vsg::RasterizationState::create());
    if (!agps.colorBlendState) pipelineStates.push_back(vsg::ColorBlendState::create());
    if (!agps.multisampleState) pipelineStates.push_back(vsg::MultisampleState::create());
    if (!agps.depthStencilState) pipelineStates.push_back(vsg::DepthStencilState::create());

    shaderHints = shaderSet->defaultShaderHints ? vsg::ShaderCompileSettings::create(*shaderSet->defaultShaderHints) : vsg::ShaderCompileSettings::create();
}

void GraphicsPipelineConfigurator::traverse(Visitor& visitor)
{
    for (auto& ps : pipelineStates) ps->accept(visitor);
    if (shaderSet) shaderSet->accept(visitor);
    if (shaderHints) shaderHints->accept(visitor);
    if (descriptorConfigurator) descriptorConfigurator->accept(visitor);
}

void GraphicsPipelineConfigurator::traverse(ConstVisitor& visitor) const
{
    for (auto& ps : pipelineStates) ps->accept(visitor);
    if (shaderSet) shaderSet->accept(visitor);
    if (shaderHints) shaderHints->accept(visitor);
    if (descriptorConfigurator) descriptorConfigurator->accept(visitor);
}

void GraphicsPipelineConfigurator::reset()
{
    pipelineStates.clear();
    shaderHints->defines.clear();
    if (descriptorConfigurator) descriptorConfigurator->reset();

    _assignShaderSetSettings();
}

struct SetPipelineStates : public Visitor
{
    uint32_t base = 0;
    const AttributeBinding& binding;
    VkVertexInputRate vir;
    uint32_t stride;
    VkFormat format;

    SetPipelineStates(uint32_t in_base, const AttributeBinding& in_binding, VkVertexInputRate in_vir, uint32_t in_stride, VkFormat in_format) :
        base(in_base),
        binding(in_binding),
        vir(in_vir),
        stride(in_stride),
        format(in_format) {}

    void apply(Object& object) override { object.traverse(*this); }
    void apply(VertexInputState& vis) override
    {
        uint32_t bindingIndex = base + static_cast<uint32_t>(vis.vertexAttributeDescriptions.size());
        vis.vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{binding.location, bindingIndex, (format != VK_FORMAT_UNDEFINED) ? format : binding.format, 0});
        vis.vertexBindingDescriptions.push_back(VkVertexInputBindingDescription{bindingIndex, stride, vir});
    }
};

bool GraphicsPipelineConfigurator::enableArray(const std::string& name, VkVertexInputRate vertexInputRate, uint32_t stride, VkFormat format)
{
    auto& attributeBinding = shaderSet->getAttributeBinding(name);
    if (attributeBinding)
    {
        if (!attributeBinding.define.empty()) shaderHints->defines.insert(attributeBinding.define);

        SetPipelineStates setVertexAttributeState(baseAttributeBinding, attributeBinding, vertexInputRate, stride, format);
        accept(setVertexAttributeState);
        return true;
    }
    return false;
}

bool GraphicsPipelineConfigurator::enableTexture(const std::string& name)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->enableTexture(name);
}

bool GraphicsPipelineConfigurator::enableDescriptor(const std::string& name)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->enableDescriptor(name);
}

bool GraphicsPipelineConfigurator::assignArray(DataList& arrays, const std::string& name, VkVertexInputRate vertexInputRate, ref_ptr<Data> array)
{
    const auto& attributeBinding = shaderSet->getAttributeBinding(name);
    if (attributeBinding)
    {
        VkFormat format = array ? array->properties.format : VK_FORMAT_UNDEFINED;

        SetPipelineStates setVertexAttributeState(baseAttributeBinding, attributeBinding, vertexInputRate, array->properties.stride, format);
        accept(setVertexAttributeState);

        arrays.push_back(array);
        return true;
    }
    return false;
}

bool GraphicsPipelineConfigurator::assignTexture(const std::string& name, ref_ptr<Data> textureData, ref_ptr<Sampler> sampler, uint32_t dstArrayElement)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->assignTexture(name, textureData, sampler, dstArrayElement);
}

bool GraphicsPipelineConfigurator::assignTexture(const std::string& name, const ImageInfoList& imageInfoList, uint32_t dstArrayElement)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->assignTexture(name, imageInfoList, dstArrayElement);
}

bool GraphicsPipelineConfigurator::assignDescriptor(const std::string& name, ref_ptr<Data> data, uint32_t dstArrayElement)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->assignDescriptor(name, data, dstArrayElement);
}

bool GraphicsPipelineConfigurator::assignDescriptor(const std::string& name, const BufferInfoList& bufferInfoList, uint32_t dstArrayElement)
{
    if (!descriptorConfigurator) descriptorConfigurator = DescriptorConfigurator::create(shaderSet);
    return descriptorConfigurator->assignDescriptor(name, bufferInfoList, dstArrayElement);
}

int GraphicsPipelineConfigurator::compare(const Object& rhs_object) const
{
    int result = Object::compare(rhs_object);
    if (result != 0) return result;

    auto& rhs = static_cast<decltype(*this)>(rhs_object);

    if ((result = compare_pointer_container(pipelineStates, rhs.pipelineStates))) return result;

    if ((result = compare_value(subpass, rhs.subpass))) return result;
    if ((result = compare_value(baseAttributeBinding, rhs.baseAttributeBinding))) return result;
    if ((result = compare_pointer(shaderSet, rhs.shaderSet))) return result;

    if ((result = compare_pointer(shaderHints, rhs.shaderHints))) return result;
    if ((result = compare_pointer_container(inheritedState, rhs.inheritedState))) return result;

    return compare_pointer(descriptorConfigurator, rhs.descriptorConfigurator);
}

void GraphicsPipelineConfigurator::assignInheritedState(const StateCommands& stateCommands)
{
    info("DescriptorConfigurator::inheritedState(", stateCommands.size(), ")");

    inheritedState = stateCommands;
}

void GraphicsPipelineConfigurator::_assignInheritedSets()
{
    inheritedSets.clear();

    struct FindInheritedState : public ConstVisitor
    {
        GraphicsPipelineConfigurator& gpc;

        explicit FindInheritedState(GraphicsPipelineConfigurator& in_gpc) :
            gpc(in_gpc) {}

        void apply(const Object& obj) override
        {
            obj.traverse(*this);
        }

        void apply(const BindDescriptorSet& bds) override
        {
            gpc.inheritedSets.insert(bds.firstSet);
        }

        void apply(const BindDescriptorSets& bds) override
        {
            gpc.inheritedSets.insert(bds.firstSet);
        }

        void apply(const BindViewDescriptorSets& bvds) override
        {
            gpc.inheritedSets.insert(bvds.firstSet);
        }
    } findInheritedState(*this);

    for (auto sc : stateCommands)
    {
        sc->accept(findInheritedState);
    }

    for (auto& is : inheritedSets)
    {
        info("   inheriting set ", is);
    }
}

void GraphicsPipelineConfigurator::init()
{
    _assignInheritedSets();

    vsg::PushConstantRanges pushConstantRanges;
    for (auto& pcb : shaderSet->pushConstantRanges)
    {
        if (pcb.define.empty()) pushConstantRanges.push_back(pcb.range);
    }

    vsg::DescriptorSetLayouts desriptorSetLayouts(shaderSet->descriptorSetRange().second);
    if (descriptorConfigurator)
    {
        descriptorConfigurator->assignDefaults(inheritedSets);

        shaderHints->defines.insert(descriptorConfigurator->defines.begin(), descriptorConfigurator->defines.end());

        for (size_t set = 0; set < descriptorConfigurator->descriptorSets.size(); ++set)
        {
            if (set >= desriptorSetLayouts.size()) desriptorSetLayouts.resize(set + 1);
            auto& ds = descriptorConfigurator->descriptorSets[set];
            if (ds) desriptorSetLayouts[set] = ds->setLayout;
        }
    }

    for (auto& cds : shaderSet->customDescriptorSetBindings)
    {
        if (cds->set >= desriptorSetLayouts.size()) desriptorSetLayouts.resize(cds->set + 1);
        desriptorSetLayouts[cds->set] = cds->createDescriptorSetLayout();
    }

    for (size_t set = 0; set < desriptorSetLayouts.size(); ++set)
    {
        auto& dsl = desriptorSetLayouts[set];
        if (!dsl)
        {
            dsl = shaderSet->createDescriptorSetLayout(shaderHints->defines, static_cast<uint32_t>(set));
        }
    }

    layout = vsg::PipelineLayout::create(desriptorSetLayouts, pushConstantRanges);
    graphicsPipeline = GraphicsPipeline::create(layout, shaderSet->getShaderStages(shaderHints), pipelineStates, subpass);
    bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);
}

bool GraphicsPipelineConfigurator::copyTo(StateCommands& stateCommands, ref_ptr<SharedObjects> sharedObjects)
{
    info("GraphicsPipelineConfigurator::copyTo()");

    bool stateAssigned = false;

    bool pipelineUnique = true;
    for(auto& sc : inheritedState)
    {
        if (compare_pointer(sc, bindGraphicsPipeline)==0) pipelineUnique = false;
    }

    if (pipelineUnique)
    {
        // create StateGroup as the root of the scene/command graph to hold the GraphicsPipeline, and binding of Descriptors to decorate the whole graph
        if (sharedObjects) sharedObjects->share(bindGraphicsPipeline);

        stateCommands.push_back(bindGraphicsPipeline);
        stateAssigned = true;

        info("   assigned unqiue ", bindGraphicsPipeline);
    }
    else
    {
        info("   pipeline NOT unqiue ", bindGraphicsPipeline);
    }


    if (descriptorConfigurator)
    {
        for (size_t set = 0; set < descriptorConfigurator->descriptorSets.size(); ++set)
        {
            if (auto ds = descriptorConfigurator->descriptorSets[set])
            {
                auto bindDescriptorSet = BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, static_cast<uint32_t>(set), ds);

                bool dsUnique = true;
                for(auto& sc : inheritedState)
                {
                    if (compare_pointer(sc, bindDescriptorSet)==0) dsUnique = false;
                }

                if (dsUnique)
                {
                    if (sharedObjects)
                    {
                        sharedObjects->share(ds);
                        sharedObjects->share(bindDescriptorSet);
                    }

                    stateCommands.push_back(bindDescriptorSet);
                    stateAssigned = true;

                    info("   descriptorset and bind descriptorset unique ", bindDescriptorSet, ", ", ds);
                }
                else
                {
                    info("   descriptorset and bind descriptorset NOT unique ", bindDescriptorSet, ", ", ds);
                }
            }
        }
    }

    for (auto& cds : shaderSet->customDescriptorSetBindings)
    {
        if (descriptorConfigurator && inheritedSets.count(cds->set) != 0)
        {
            info("GraphicsPipelineConfigurator::copyTo(..) no need to assign CustomDescriptorSetBinding on set ", cds->set);
            continue;
        }

        if (auto sc = cds->createStateCommand(layout))
        {
            if (sharedObjects)
            {
                sharedObjects->share(sc);
            }
            stateCommands.push_back(sc);
            stateAssigned = true;
        }
    }

    return stateAssigned;
}

bool GraphicsPipelineConfigurator::copyTo(ref_ptr<StateGroup> stateGroup, ref_ptr<SharedObjects> sharedObjects)
{
    if (copyTo(stateGroup->stateCommands, sharedObjects))
    {
        // assign any custom ArrayState that may be required.
        stateGroup->prototypeArrayState = shaderSet->getSuitableArrayState(shaderHints->defines);

        return true;
    }
    else
    {
        return false;
    }
}
