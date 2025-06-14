#pragma once

#ifdef PLATFORM_WINDOWS
#   include "core/device/device_manager_dx12.hpp"
#endif

#include "core/uuid.hpp"
#include "core/vfs/vfs.hpp"
#include "core/application.hpp"
#include "core/base.hpp"
#include "core/device/device_manager.hpp"
#include "core/device/device_manager_vk.hpp"
#include "core/input/event.hpp"
#include "core/input/app_event.hpp"
#include "core/input/key_event.hpp"
#include "core/input/mouse_event.hpp"
#include "core/input/key_codes.hpp"
#include "core/input/mouse_codes.hpp"
#include "core/layer.hpp"
#include "core/layer_stack.hpp"
#include "core/logger.hpp"
#include "core/string_utils.hpp"
#include "core/types.hpp"
#include "core/time.hpp"
#include "core/input/input.hpp"

#include "asset/asset.hpp"

#include "imgui/imgui_nvrhi.hpp"
#include "imgui/imgui_layer.hpp"

#include "graphics/renderer.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer_2d.hpp"
#include "graphics/shader_factory.hpp"
#include "graphics/window.hpp"
#include "graphics/mesh.hpp"
#include "graphics/mesh_factory.hpp"

#include "math/math.hpp"
#include "math/aabb.hpp"
#include "math/obb.hpp"

#include "physics/2d/physics_2d.hpp"
#include "physics/2d/physics_2d_component.hpp"
#include "project/project.hpp"

#include "scene/component.hpp"
#include "scene/Icamera.hpp"
#include "scene/entity.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
