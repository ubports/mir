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

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "native_buffer.h"
#include "buffer_allocator.h"
#include "ipc_operations.h"
#include "mesa_extensions.h"

#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;

//TODO replace printf debug messages

const wl_registry_listener mgw::Platform::registry_listener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        (void)version;
        mgw::Platform *dp = static_cast<mgw::Platform *>(data);
        printf("Got a registry event for %s id %d\n", interface, id);
          if (strcmp(interface, "wl_compositor") == 0) {
              dp->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry,
      				      id,
      				      &wl_compositor_interface,
      				      1));
          } else if (strcmp(interface, "wl_shell") == 0) {
              dp->shell = static_cast<wl_shell*>(wl_registry_bind(registry, id,
                                       &wl_shell_interface, 1));
          }
    },
    [](void *, wl_registry *registry, uint32_t id) {
      (void)registry;
      printf("Got a registry losing event for %d\n", id);
    }
};

mgw::Platform::Platform(std::shared_ptr<mg::DisplayReport> const& report)
    : display{EGL_NO_DISPLAY},
      report{report}
{
  using namespace std::literals;

  w_display = static_cast<wl_display*>(wl_display_connect(NULL));
  if (w_display == NULL)
  {
      BOOST_THROW_EXCEPTION(mg::egl_error("Failed to connect to wayland"));
  }

	registry = wl_display_get_registry(w_display);
	wl_registry_add_listener(registry,
				 &registry_listener, this);

	wl_display_roundtrip(w_display);

  display = eglGetDisplay(w_display);

  surface = wl_compositor_create_surface(compositor);
  shell_surface = wl_shell_get_shell_surface(shell, surface);
//  wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, window);
  wl_shell_surface_set_fullscreen(shell_surface,
        WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
        0, NULL);
  wl_shell_surface_set_toplevel(shell_surface);
  w_window = wl_egl_window_create(surface, 250, 250);

  if (display == EGL_NO_DISPLAY)
  {
      BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get wayland EGLDisplay"));
  }

  EGLint major{1};
  EGLint minor{4};
  auto const required_egl_version_major = major;
  auto const required_egl_version_minor = minor;
  if (eglInitialize(display, &major, &minor) != EGL_TRUE)
  {
      BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL"));
  }
  if ((major < required_egl_version_major) ||
      (major == required_egl_version_major && minor < required_egl_version_minor))
  {
      BOOST_THROW_EXCEPTION((std::runtime_error{
          "Incompatible EGL version"s +
          "Wanted 1.4, got " + std::to_string(major) + "." + std::to_string(minor)}));
  }
}

mir::UniqueModulePtr<mg::Display> mgw::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const& gl_config)
{
  std::shared_ptr<EGLNativeWindowType> const& win = std::make_shared<EGLNativeWindowType>(reinterpret_cast<EGLNativeWindowType>(w_window));
  std::shared_ptr<EGLDisplay> const& disp = std::make_shared<EGLDisplay>(display);
  return mir::make_module_ptr<mgw::Display>(disp, gl_config, report, win);
}

mg::NativeDisplayPlatform* mgw::Platform::native_display_platform()
{
    printf("%s\n", __FUNCTION__);
    return nullptr;
}

EGLNativeDisplayType mgw::Platform::egl_native_display() const
{
    printf("%s\n", __FUNCTION__);
    return display;
}


std::vector<mir::ExtensionDescription> mgw::Platform::extensions() const
{
    printf("%s\n", __FUNCTION__);
    return
    {
        { "mir_extension_graphics_module", { 1 } }
    };
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgw::Platform::create_buffer_allocator()
{
    printf("%s\n", __FUNCTION__);
    return mir::make_module_ptr<mgw::BufferAllocator>();
}

mg::NativeRenderingPlatform* mgw::Platform::native_rendering_platform()
{
  printf("%s\n", __FUNCTION__);
    return this;
}
mir::UniqueModulePtr<mg::PlatformIpcOperations> mgw::Platform::make_ipc_operations() const
{
    class NoIPCOperations : public mg::PlatformIpcOperations
    {

    public:
        void pack_buffer(BufferIpcMessage& packer, Buffer const& buffer, BufferIpcMsgType msg_type) const override
        {
            if (msg_type == mg::BufferIpcMsgType::full_msg)
            {
                auto native_handle = std::dynamic_pointer_cast<mgw::NativeBuffer>(buffer.native_buffer_handle());
                if (!native_handle)
                    BOOST_THROW_EXCEPTION(std::invalid_argument{"could not convert NativeBuffer"});
                for(auto i=0; i<native_handle->data_items; i++)
                {
                    packer.pack_data(native_handle->data[i]);
                }
                for(auto i=0; i<native_handle->fd_items; i++)
                {
                    packer.pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
                }

                packer.pack_stride(mir::geometry::Stride{native_handle->stride});
                packer.pack_flags(native_handle->flags);
                packer.pack_size(buffer.size());
            }
        }

        void unpack_buffer(BufferIpcMessage& /*message*/, Buffer const& /*buffer*/) const override
        {

        }

        std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
        {
            return std::make_shared<mg::PlatformIPCPackage>(describe_graphics_module());
        }

        PlatformOperationMessage platform_operation(unsigned int const /*opcode*/,
            PlatformOperationMessage const& /*message*/) override
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"No platform operations implemented"});
        }
    };
    return mir::make_module_ptr<NoIPCOperations>();
}
