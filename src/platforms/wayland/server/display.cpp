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

#include <epoxy/egl.h>

#include "display.h"
#include "egl_helper.h"
#include "display_buffer.h"
#include "display_configuration.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/atomic_frame.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/virtual_output.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/transformation.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/renderer/gl/context.h"

#include <sys/ioctl.h>
#include <system_error>
#include <boost/throw_exception.hpp>
#include <wayland-egl.h>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
namespace geom=mir::geometry;

mgw::Display::Display(
    std::shared_ptr<EGLDisplay> const& dpy,
    std::shared_ptr<GLConfig> const& gl_config,
    std::shared_ptr<DisplayReport> const& report,
    std::shared_ptr<EGLNativeWindowType> const& window)
    : display{*dpy.get()},
      shared_egl{*gl_config, display},
      gl_config{gl_config},
      last_frame{std::make_shared<AtomicFrame>()},
      actual_size{geom::Size{250, 250}},
      pixel_width{250},
      pixel_height{250},
      scale{1.0f},
      orientation{mir_orientation_normal}
{
  shared_egl.setup();

  pf = mir_pixel_format_argb_8888;

  display_buffer = std::make_unique<mgw::DisplayBuffer>(
                       display,
                       window,
                       actual_size,
                       shared_egl.context(),
                       last_frame,
                       report,
                       *gl_config);

  shared_egl.make_current();
  report->report_successful_display_construction();

}

void mgw::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
  printf("In for_each_display_sync_group\n");
  f(*display_buffer);
}

std::unique_ptr<mg::DisplayConfiguration> mgw::Display::configuration() const
{
    printf("In configuration\n");
    return std::make_unique<mgw::DisplayConfiguration>(
        pf, actual_size, geom::Size{actual_size.width * pixel_width, actual_size.height * pixel_height}, scale, orientation);
}

void mgw::Display::configure(mg::DisplayConfiguration const& conf)
{
  printf("In configure\n");
  if (!conf.valid())
  {
      BOOST_THROW_EXCEPTION(
          std::logic_error("Invalid or inconsistent display configuration"));
  }

  MirOrientation o = mir_orientation_normal;
  float new_scale = scale;
  geom::Rectangle logical_area;

  conf.for_each_output([&](DisplayConfigurationOutput const& conf_output)
  {
      o = conf_output.orientation;
      new_scale = conf_output.scale;
      logical_area = conf_output.extents();
  });

  orientation = o;
  display_buffer->set_view_area(logical_area);
  display_buffer->set_transformation(mg::transformation(orientation));
  scale = new_scale;
}

void mgw::Display::register_configuration_change_handler(
    EventHandlerRegister& /*handlers*/,
    DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
}

void mgw::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgw::Display::pause()
{
   BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on wayland platform"));
}

void mgw::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on wayland platform"));
}

std::shared_ptr<mg::Cursor> mgw::Display::create_hardware_cursor()
{
    // TODO: wayland cursor
    return nullptr;
}

std::unique_ptr<mg::VirtualOutput> mgw::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}

mg::NativeDisplay* mgw::Display::native_display()
{
    return this;
}

std::unique_ptr<mir::renderer::gl::Context> mgw::Display::create_gl_context()
{
  class GLContext : public mir::renderer::gl::Context
  {
  public:
      GLContext(EGLDisplay& dpy,
                 std::shared_ptr<mg::GLConfig> const& gl_config,
                 EGLContext const shared_ctx)
          : egl{*gl_config, dpy}
      {
          egl.setup(shared_ctx);
      }

      ~GLContext() = default;

      void make_current() const override
      {
          egl.make_current();
      }

      void release_current() const override
      {
          egl.release_current();
      }

  private:
      mgw::helpers::EGLHelper egl;
  };
  printf("In GLContext\n");
  return std::make_unique<GLContext>(display, gl_config, shared_egl.context());
}

bool mgw::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& /*conf*/)
{
    return false;
}

mg::Frame mgw::Display::last_frame_on(unsigned) const
{
  return last_frame->load();
}
