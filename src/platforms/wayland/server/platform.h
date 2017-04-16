/*
 * Copyright © 2017 The Ubports project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Marius Gripsgard <marius@ubports.com>
 */

#ifndef MIR_PLATFORMS_WAYLAND_PLATFORM_H_
#define MIR_PLATFORMS_WAYLAND_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/fd.h"
#include "mir/renderer/gl/egl_platform.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace mir
{
namespace graphics
{
namespace wayland
{
class Platform : public graphics::Platform,
                 public graphics::NativeRenderingPlatform,
                 public mir::renderer::gl::EGLPlatform
{
public:
    explicit Platform(std::shared_ptr<DisplayReport> const& report);
    ~Platform() = default;

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
        std::shared_ptr<GLConfig> const& /*gl_config*/) override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;

    NativeRenderingPlatform* native_rendering_platform() override;
    EGLNativeDisplayType egl_native_display() const override;
    static const wl_registry_listener registry_listener;

private:
    EGLDisplay display;
    std::shared_ptr<DisplayReport> const report;
    struct wl_display *w_display;
    struct wl_egl_window *w_window;
    struct wl_compositor *compositor = NULL;
    struct wl_surface *surface;
    struct wl_shell *shell;
    struct wl_shell_surface *shell_surface;
    struct wl_registry *registry;
};
}
}
}

#endif // MIR_PLATFORMS_WAYLAND_PLATFORM_H_
