#pragma once

#include <vulkan/vulkan_core.h>

namespace vke {

class Core;
class Resource;

class Surface;
class Window;
class Window_SDL;

class Buffer;
class IBufferSpan;

class CommandBuffer;
class Renderpass;
class Fence;
class Semaphore;
class Pipeline;
class Image;
class DescriptorPool;

class RenderEngine;

class MaterialManager;
class Material;

class IRenderTarget;
class IStencilBuffer;

class ArenaAllocator;

template <class... TArgs>
class EventManager;

} // namespace vke