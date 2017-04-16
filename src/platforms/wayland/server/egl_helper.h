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

#ifndef MIR_GRAPHICS_WAYLAND_EGL_HELPER_H_
#define MIR_GRAPHICS_WAYLAND_EGL_HELPER_H_

#include <memory>

#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace mir
{
namespace graphics
{
class GLConfig;

namespace wayland
{

namespace helpers
{

class EGLHelper
{
public:
    EGLHelper(GLConfig const& gl_config, EGLDisplay& dpy);
    ~EGLHelper() noexcept;

    EGLHelper(const EGLHelper&) = delete;
    EGLHelper& operator=(const EGLHelper&) = delete;

    void setup();
    void setup(EGLContext shared_context);
    void setup(std::shared_ptr<EGLNativeWindowType> const& egl_window,
               EGLContext shared_context);

    bool swap_buffers();
    bool make_current() const;
    bool release_current() const;
    void draw_example();

    EGLContext context() { return egl_context; }
    EGLDisplay display() { return egl_display; }
    EGLConfig config() { return egl_config; }
    EGLSurface surface() const { return egl_surface; }

    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)>);
private:
    void setup_internal(bool initialize);

    EGLint const depth_buffer_bits;
    EGLint const stencil_buffer_bits;
    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;
    EGLSurface egl_surface;
    bool should_terminate_egl;
};

}
}
}
}
#endif /* MIR_GRAPHICS_WAYLAND_EGL_HELPER_H_ */
