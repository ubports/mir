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

#ifndef MIR_PLATFORMS_WAYLAND_DISPLAY_H_
#define MIR_PLATFORMS_WAYLAND_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/atomic_frame.h"
#include "mir/fd.h"
#include "mir/renderer/gl/context_source.h"
#include "egl_helper.h"
#include "display_buffer.h"
#include <wayland-egl.h>

namespace mir
{
namespace graphics
{
class DisplayConfigurationPolicy;
class GLConfig;
class AtomicFrame;

namespace wayland
{
class Display : public mir::graphics::Display,
                public mir::graphics::NativeDisplay,
                public mir::renderer::gl::ContextSource
{
public:
    Display(
        std::shared_ptr<EGLDisplay> const& display,
        std::shared_ptr<GLConfig> const& gl_config,
        std::shared_ptr<DisplayReport> const& report,
        std::shared_ptr<EGLNativeWindowType> const& window);

    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;

    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler, DisplayResumeHandler const& resume_handler) override;

    void pause() override;

    void resume() override;

    std::shared_ptr<Cursor> create_hardware_cursor() override;

    std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) override;

    NativeDisplay* native_display() override;

    std::unique_ptr<renderer::gl::Context> create_gl_context() override;
    Frame last_frame_on(unsigned output_id) const override;

private:
    EGLDisplay display;
    helpers::EGLHelper shared_egl;
    EGLConfig config;
    EGLContext context;
    std::shared_ptr<GLConfig> const gl_config;
    std::shared_ptr<AtomicFrame> last_frame;
    mir::geometry::Size const actual_size;
    float pixel_width;
    float pixel_height;
    float scale;
    std::shared_ptr<DisplayConfigurationPolicy> const configuration_policy;
    std::unique_ptr<DisplayBuffer> display_buffer;
    MirPixelFormat pf;
    MirOrientation orientation;
};

}
}

}

#endif  // MIR_PLATFORMS_WAYLAND_DISPLAY_H_
