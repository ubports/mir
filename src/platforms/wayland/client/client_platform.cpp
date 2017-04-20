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

#include "mir_toolkit/mir_client_library.h"
#include "client_platform.h"
#include "client_buffer_factory.h"
#include "mir/weak_egl.h"
#include "mir/client_buffer_factory.h"
#include "mir/client_context.h"
#include "native_buffer.h"
#include "wayland_native_display_container.h"

#include <cstring>
#include <gbm.h>
#include <boost/throw_exception.hpp>

namespace mcl=mir::client;
namespace mclw=mir::client::wayland;
namespace geom=mir::geometry;

namespace {
  struct NativeDisplayDeleter
  {
      NativeDisplayDeleter(mcl::EGLNativeDisplayContainer& container)
      : container(container)
      {
      }
      void operator() (EGLNativeDisplayType* p)
      {
          auto display = *(reinterpret_cast<MirEGLNativeDisplayType*>(p));
          container.release(display);
          delete p;
      }

      mcl::EGLNativeDisplayContainer& container;
  };
}

mclw::ClientPlatform::ClientPlatform(ClientContext* const,
                                      mcl::EGLNativeDisplayContainer& display_container):
                                      display_container(display_container)
{
}

std::shared_ptr<mcl::ClientBufferFactory> mclw::ClientPlatform::create_buffer_factory()
{
    return std::make_shared<mclw::ClientBufferFactory>();
}

void mclw::ClientPlatform::use_egl_native_window(std::shared_ptr<void> /*native_window*/, EGLNativeSurface* /*surface*/)
{
  printf("use_egl_native_window\n");
}

std::shared_ptr<void> mclw::ClientPlatform::create_egl_native_window(EGLNativeSurface* /*surface*/)
{
    printf("create_egl_native_window\n");
    return nullptr;
}

std::shared_ptr<EGLNativeDisplayType> mclw::ClientPlatform::create_egl_native_display()
{
  printf("create_egl_native_display\n");
  MirEGLNativeDisplayType *mir_native_display = new MirEGLNativeDisplayType;
  *mir_native_display = display_container.create(this);
  auto egl_native_display = reinterpret_cast<EGLNativeDisplayType*>(mir_native_display);

  return std::shared_ptr<EGLNativeDisplayType>(egl_native_display, NativeDisplayDeleter(display_container));

}

MirPlatformType mclw::ClientPlatform::platform_type() const
{
    return mir_platform_type_wayland;
}

void mclw::ClientPlatform::populate(MirPlatformPackage& /*package*/) const
{
}

MirPlatformMessage* mclw::ClientPlatform::platform_operation(MirPlatformMessage const* /*msg*/)
{
  printf("platform_operation\n");
    return nullptr;
}

MirNativeBuffer* mclw::ClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
  printf("convert_native_buffer");
    if (auto native = dynamic_cast<mir::graphics::wayland::NativeBuffer*>(buf))
        return native;
    BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert to NativeBuffer"));
}


MirPixelFormat mclw::ClientPlatform::get_egl_pixel_format(
    EGLDisplay disp, EGLConfig conf) const
{
  printf("get_egl_pixel_format\n");
  MirPixelFormat mir_format = mir_pixel_format_invalid;

  /*
   * This is based on gbm_dri_is_format_supported() however we can't call it
   * via the public API gbm_device_is_format_supported because that is
   * too buggy right now (LP: #1473901).
   *
   * Ideally Mesa should implement EGL_NATIVE_VISUAL_ID for all platforms
   * to explicitly return the exact GBM pixel format. But it doesn't do that
   * yet (for most platforms). It does however successfully return zero for
   * EGL_NATIVE_VISUAL_ID, so ignore that for now.
   */
  EGLint r = 0, g = 0, b = 0, a = 0;
  mcl::WeakEGL weak;
  weak.eglGetConfigAttrib(disp, conf, EGL_RED_SIZE, &r);
  weak.eglGetConfigAttrib(disp, conf, EGL_GREEN_SIZE, &g);
  weak.eglGetConfigAttrib(disp, conf, EGL_BLUE_SIZE, &b);
  weak.eglGetConfigAttrib(disp, conf, EGL_ALPHA_SIZE, &a);

  if (r == 8 && g == 8 && b == 8)
  {
      // GBM is very limited, which at least makes this simple...
      if (a == 8)
          mir_format = mir_pixel_format_argb_8888;
      else if (a == 0)
          mir_format = mir_pixel_format_xrgb_8888;
  }

  return mir_format;
}

void* mclw::ClientPlatform::request_interface(char const*, int)
{
  printf("request_interface\n");
    return nullptr;
}

uint32_t mclw::ClientPlatform::native_format_for(MirPixelFormat format) const
{
  printf("native_format_for\n");
  uint32_t gbm_pf;

  switch (format)
  {
  case mir_pixel_format_argb_8888:
      gbm_pf = GBM_FORMAT_ARGB8888;
      break;
  case mir_pixel_format_xrgb_8888:
      gbm_pf = GBM_FORMAT_XRGB8888;
      break;
  case mir_pixel_format_abgr_8888:
      gbm_pf = GBM_FORMAT_ABGR8888;
      break;
  case mir_pixel_format_xbgr_8888:
      gbm_pf = GBM_FORMAT_XBGR8888;
      break;
  case mir_pixel_format_bgr_888:
      gbm_pf = GBM_FORMAT_BGR888;
      break;
  case mir_pixel_format_rgb_888:
      gbm_pf = GBM_FORMAT_RGB888;
      break;
  case mir_pixel_format_rgb_565:
      gbm_pf = GBM_FORMAT_RGB565;
      break;
  case mir_pixel_format_rgba_5551:
      gbm_pf = GBM_FORMAT_RGBA5551;
      break;
  case mir_pixel_format_rgba_4444:
      gbm_pf = GBM_FORMAT_RGBA4444;
      break;
  default:
      printf("INVALID FORMAT\n");
      break;
  }

  return gbm_pf;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
uint32_t mclw::ClientPlatform::native_flags_for(MirBufferUsage, mir::geometry::Size size) const
{
  printf("native_flags_for\n");
  uint32_t bo_flags{GBM_BO_USE_RENDERING};
  if (size.width.as_uint32_t() >= 800 && size.height.as_uint32_t() >= 600)
      bo_flags |= GBM_BO_USE_SCANOUT;
  return bo_flags;
}
#pragma GCC diagnostic pop
