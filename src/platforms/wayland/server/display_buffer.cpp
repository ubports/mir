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

#include "mir/graphics/atomic_frame.h"
#include "mir/fatal.h"
#include "display_buffer.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/transformation.h"
#include <cstring>
#include <wayland-egl.h>
#include <wayland-client.h>

namespace mg=mir::graphics;
namespace mgw=mg::wayland;
namespace geom=mir::geometry;

mgw::DisplayBuffer::DisplayBuffer(EGLDisplay& dpy,
                                  std::shared_ptr<EGLNativeWindowType> const& window,
                                  geometry::Size const& view_area_size,
                                  EGLContext const shared_context,
                                  std::shared_ptr<AtomicFrame> const& f,
                                  std::shared_ptr<DisplayReport> const& r,
                                  GLConfig const& gl_config)
                                  : report{r},
                                    area{{0,0}, view_area_size},
                                    egl{gl_config, dpy},
                                    last_frame{f}
{
    egl.setup(window, shared_context);
    egl.report_egl_configuration(
      [&r] (EGLDisplay disp, EGLConfig cfg)
      {
          r->report_egl_configuration(disp, cfg);
      });

    printf("%s\n", __PRETTY_FUNCTION__);
}

void mgw::DisplayBuffer::make_current()
{
  printf("%s\n", __PRETTY_FUNCTION__);
    if (!egl.make_current())
        fatal_error("Failed to make EGL surface current");
}

void mgw::DisplayBuffer::release_current()
{
  printf("%s\n", __PRETTY_FUNCTION__);
    egl.release_current();
}

bool mgw::DisplayBuffer::overlay(RenderableList const& /*renderlist*/)
{
    return false;
}

void mgw::DisplayBuffer::swap_buffers()
{
  printf("%s\n", __PRETTY_FUNCTION__);
    if (!egl.swap_buffers())
        fatal_error("Failed to perform buffer swap");

    /*
     * It would be nice to call this on demand as required. However the
     * implementation requires an EGL context. So for simplicity we call it here
     * on every frame.
     *   This does mean the current last_frame will be a bit out of date if
     * the compositor missed a frame. But that doesn't actually matter because
     * the consequence of that would be the client scheduling the next frame
     * immediately without waiting, which is probably ideal anyway.
     */
    int64_t ust_us, msc, sbc;
    last_frame->increment_now();

    /*
     * Admittedly we are not a real display and will miss some real vsyncs
     * but this is best-effort. And besides, we don't want Mir reporting all
     * real vsyncs because that would mean the compositor never sleeps.
     */
    report->report_vsync(mgw::DisplayConfiguration::the_output_id.as_value(),
                         last_frame->load());
}

void mgw::DisplayBuffer::bind()
{
}

void mgw::DisplayBuffer::set_view_area(geom::Rectangle const& a)
{
  area = a;
}

geom::Rectangle mgw::DisplayBuffer::view_area() const
{
    return area;
}

glm::mat2 mgw::DisplayBuffer::transformation() const
{
    return transform;
}

void mgw::DisplayBuffer::set_transformation(glm::mat2 const& t)
{
    transform = t;
}

mg::NativeDisplayBuffer* mgw::DisplayBuffer::native_display_buffer()
{
  printf("%s\n", __PRETTY_FUNCTION__);
    return this;
}

void mgw::DisplayBuffer::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
  printf("%s\n", __PRETTY_FUNCTION__);
    f(*this);
}

void mgw::DisplayBuffer::post()
{
}

std::chrono::milliseconds mgw::DisplayBuffer::recommended_sleep() const
{
  printf("%s\n", __PRETTY_FUNCTION__);
    return std::chrono::milliseconds::zero();
}
