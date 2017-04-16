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

#ifndef MIR_GRAPHICS_WAYLAND_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_WAYLAND_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/render_target.h"
#include "egl_helper.h"

#include <EGL/egl.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include <memory>

namespace mir
{
namespace graphics
{

class AtomicFrame;
class GLConfig;
class DisplayReport;

namespace wayland
{

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::DisplaySyncGroup,
                      public graphics::NativeDisplayBuffer,
                      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(
            EGLDisplay& dpy,
            std::shared_ptr<EGLNativeWindowType> const& window,
            geometry::Size const& view_area_size,
            EGLContext const shared_context,
            std::shared_ptr<AtomicFrame> const& f,
            std::shared_ptr<DisplayReport> const& r,
            GLConfig const& gl_config);

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;
    bool overlay(RenderableList const& renderlist) override;
    void set_view_area(geometry::Rectangle const& a);
    void set_transformation(glm::mat2 const& t);

    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    glm::mat2 transformation() const override;
    NativeDisplayBuffer* native_display_buffer() override;

private:
    std::shared_ptr<DisplayReport> const report;
    glm::mat2 transform;
    geometry::Rectangle area;
    helpers::EGLHelper egl;
    std::shared_ptr<AtomicFrame> const last_frame;
};

}
}
}

#endif /* MIR_GRAPHICS_WAYLAND_DISPLAY_BUFFER_H_ */
