#pragma once

namespace vke {

class Camera;
class PerspectiveCamera;
class OrthographicCamera;
class FreeCamera;
class RenderServer;
class Mesh;
class MeshRenderer;
class LineDrawer;
class SceneSet;
class IObjectRenderer;
class ObjectRenderer;
class Scene;
class GameEngine;
class DeferredRenderPipeline;
class ResourceManager;
class RenderSystem;
class IShadowMap;
class DirectShadowMap;
class ShadowManager;
class GPUTimingSystem;
class HierarchicalZBuffers;
class SceneBuffersManager;

// components
class Transform;
// ui
class IMenu;
class InstantiateMenu;
} // namespace vke

namespace flecs {
struct world;
struct entity;
}