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

#include "platform.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/log.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <sstream>
#include <wayland-client.h>

#include <fcntl.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mgw = mir::graphics::wayland;

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    return mir::make_module_ptr<mgw::Platform>(report);
}


void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

mg::PlatformPriority probe_graphics_platform(mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);

   if (wl_display_connect(NULL) == NULL)
        return mg::PlatformPriority::unsupported;
    return mg::PlatformPriority::best;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:wayland",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

mir::UniqueModulePtr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mg::PlatformAuthentication> const& /*platform_authentication*/)
{
    mir::assert_entry_point_signature<mg::CreateGuestPlatform>(&create_guest_platform);
    return nullptr;
}
