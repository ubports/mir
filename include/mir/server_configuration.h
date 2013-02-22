/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include "mir/options/program_option.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferAllocationStrategy;
class GraphicBufferAllocator;
class BufferBundleFactory;
class BufferBundleManager;
class RenderView;
}
namespace frontend
{
class Communicator;
class ProtobufIpcFactory;
class ApplicationListener;
}

namespace sessions
{
class SessionStore;
class SurfaceFactory;
}
namespace surfaces
{
class SurfaceStackModel;
class SurfaceStack;
}
namespace graphics
{
class Renderer;
class Platform;
class Display;
class ViewableArea;
class BufferInitializer;
}
namespace input
{
class InputManager;
class EventFilter;
}

class ServerConfiguration
{
public:
    virtual std::shared_ptr<graphics::Platform> the_graphics_platform() = 0;
    virtual std::shared_ptr<graphics::BufferInitializer> the_buffer_initializer() = 0;
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> the_buffer_allocation_strategy() = 0;
    virtual std::shared_ptr<graphics::Renderer> the_renderer() = 0;
    virtual std::shared_ptr<frontend::Communicator> the_communicator(
        std::shared_ptr<sessions::SessionStore> const& session_store) = 0;
    virtual std::shared_ptr<sessions::SessionStore> the_session_store(
        std::shared_ptr<sessions::SurfaceFactory> const& surface_factory) = 0;
    virtual std::shared_ptr<input::InputManager> the_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters) = 0;
    virtual std::shared_ptr<compositor::GraphicBufferAllocator> the_buffer_allocator() = 0;
    virtual std::shared_ptr<graphics::Display> the_display() = 0;
    virtual std::shared_ptr<compositor::BufferBundleFactory> the_buffer_bundle_factory() = 0;
    virtual std::shared_ptr<surfaces::SurfaceStackModel> the_surface_stack_model() = 0;
    virtual std::shared_ptr<compositor::RenderView> the_render_view() = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() {}

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};

class DefaultServerConfiguration : public ServerConfiguration
{
public:
    DefaultServerConfiguration(int argc, char const* argv[]);

    virtual std::shared_ptr<graphics::Platform> the_graphics_platform();
    virtual std::shared_ptr<graphics::BufferInitializer> the_buffer_initializer();
    virtual std::shared_ptr<compositor::BufferAllocationStrategy> the_buffer_allocation_strategy();
    virtual std::shared_ptr<graphics::Renderer> the_renderer();
    virtual std::shared_ptr<frontend::Communicator> the_communicator(
        std::shared_ptr<sessions::SessionStore> const& session_store);
    virtual std::shared_ptr<sessions::SessionStore> the_session_store(
        std::shared_ptr<sessions::SurfaceFactory> const& surface_factory);
    virtual std::shared_ptr<input::InputManager> the_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters);
    virtual std::shared_ptr<compositor::GraphicBufferAllocator> the_buffer_allocator();
    virtual std::shared_ptr<graphics::Display> the_display();
    virtual std::shared_ptr<compositor::BufferBundleFactory> the_buffer_bundle_factory();
    virtual std::shared_ptr<surfaces::SurfaceStackModel> the_surface_stack_model();
    virtual std::shared_ptr<compositor::RenderView> the_render_view();

protected:
    // TODO deprecated
    explicit DefaultServerConfiguration();
    virtual std::shared_ptr<options::Option> the_options() const;

    template<typename Type>
    class CachedPtr
    {
        std::weak_ptr<Type> cache;

        CachedPtr(CachedPtr const&) = delete;
        CachedPtr& operator=(CachedPtr const&) = delete;
    public:
        CachedPtr() = default;

        std::shared_ptr<Type> operator()(std::function<std::shared_ptr<Type>()> make)
        {
            auto result = cache.lock();
            if (!result)
            {
                cache = result = make();
            }

            return result;

        }
    };

    CachedPtr<frontend::Communicator> communicator;
    CachedPtr<sessions::SessionStore> session_store;
    CachedPtr<input::InputManager>    input_manager;
    CachedPtr<graphics::Platform>     graphics_platform;
    CachedPtr<graphics::BufferInitializer> buffer_initializer;
    CachedPtr<compositor::GraphicBufferAllocator> buffer_allocator;
    CachedPtr<graphics::Display>      display;

    CachedPtr<frontend::ProtobufIpcFactory>  ipc_factory;
    CachedPtr<frontend::ApplicationListener> application_listener;
    CachedPtr<compositor::BufferAllocationStrategy> buffer_allocation_strategy;
    CachedPtr<graphics::Renderer> renderer;
    CachedPtr<compositor::BufferBundleManager> buffer_bundle_manager;
    CachedPtr<surfaces::SurfaceStack> surface_stack;

private:
    std::shared_ptr<options::Option> options;

    // the communications interface to use
    virtual std::shared_ptr<frontend::ProtobufIpcFactory> the_ipc_factory(
        std::shared_ptr<sessions::SessionStore> const& session_store,
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<compositor::GraphicBufferAllocator> const& allocator);

    virtual std::shared_ptr<frontend::ApplicationListener> the_application_listener();

    virtual std::string the_socket_file() const;
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
