#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glbinding/gl/types.h>

#include <Deliberation/Core/DataLayout.h>
#include <Deliberation/Core/LayoutedBlob.h>

#include <Deliberation/Draw/Detail/UniformImpl.h>
#include <Deliberation/Draw/Detail/SamplerImpl.h>
#include <Deliberation/Draw/Detail/VertexAttributeBinding.h>
#include <Deliberation/Draw/DrawState.h>
#include <Deliberation/Draw/Framebuffer.h>
#include <Deliberation/Draw/FramebufferBinding.h>
#include <Deliberation/Draw/VertexAttribute.h>

#include "BufferBinding.h"
#include "UniformBufferBinding.h"

namespace deliberation
{
class DrawContext;
class Program;
class ProgramImpl;

class DrawImpl final
{
  public:
    struct UniformBinding
    {
        size_t count = 0;
        bool   assigned = false;
    };

  public:
    DrawImpl(DrawContext & drawContext, const Program & program);
    ~DrawImpl();

    void setAttribute(
        const ProgramInterfaceVertexAttribute & attribute, const void * data);

    DrawContext &                drawContext;
    std::string                  name;
    std::shared_ptr<ProgramImpl> program;
    BufferBinding                indexBufferBinding;
    bool                         indexBufferBindingDirty = true;

    std::vector<gl::GLuint>      bufferTextures;

    /**
     * Vertex attributes
     */
    std::vector<VertexAttributeBinding> attributeBindings;
    std::vector<u32>                    dirtyValueAttributes;
    Blob                                valueAttributes;

    DrawState state;

    FramebufferBinding framebufferBinding;

    DataLayout                  uniformLayout;
    LayoutedBlob                uniformData;
    std::vector<UniformBinding> uniforms;

    gl::GLuint                                  glVertexArray = 0;
    std::vector<Optional<UniformBufferBinding>> uniformBuffers;

    std::vector<std::shared_ptr<SamplerImpl>> samplers;
};
}
