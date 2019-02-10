/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "platform.h"

#include "display.h"
#include "graphic_buffer_allocator.h"
#include "hal_component_factory.h"
#include "hwc_loggers.h"
#include "ipc_operations.h"
#include "native_buffer.h"
#include "native_window_report.h"
#include "resource_factory.h"
#include "sync_fence.h"

#include "mir/abnormal_exit.h"
#include "mir/assert_module_entry_point.h"
#include "mir/gl/default_program_factory.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/libname.h"
#include "mir/logging/dumb_console_logger.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

#include <boost/throw_exception.hpp>
#include <mutex>
#include <stdexcept>
#include <string.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace mf = mir::frontend;
namespace mo = mir::options;

namespace
{
char const* const hwc_log_opt = "hwc-report";
char const* const hwc_overlay_opt = "disable-overlays";
char const* const fb_native_window_report_opt = "report-fb-native-window";

bool force_caf_version() {
    char value[PROP_VALUE_MAX] = "";
    return 0 != ::property_get("ro.build.qti_bsp.abi", value, nullptr);
}

#ifdef ANDROID_CAF
bool force_vanilla_version() {
    char value[PROP_VALUE_MAX] = "";
    return 0 != ::property_get("ro.build.vanilla.abi", value, nullptr);
}
std::tuple<int, int, int> get_android_version()
{
    char value[PROP_VALUE_MAX] = "";
    char const key[] = "ro.build.version.release";
    ::property_get(key, value, "4.1.1");

    std::tuple<int, int, int> ret{0, 0, 0};
    if (sscanf(value, "%d.%d.%d", &std::get<0>(ret), &std::get<1>(ret), &std::get<2>(ret)) == 3)
        return ret;
    else
        return std::make_tuple(4, 1, 1);
}
#endif

std::shared_ptr<mga::HwcReport> make_hwc_report(mo::Option const& options)
{
    if (!options.is_set(hwc_log_opt))
        return std::make_shared<mga::NullHwcReport>();

    auto opt = options.get<std::string>(hwc_log_opt);
    if (opt == mo::log_opt_value)
        return std::make_shared<mga::HwcFormattedLogger>();
    else if (opt == mo::off_opt_value)
        return std::make_shared<mga::NullHwcReport>();
    else
        throw mir::AbnormalExit(std::string("Invalid hwc-report option: " + opt + " (valid options are: \"" +
                                            mo::off_opt_value + "\" and \"" + mo::log_opt_value + "\")"));
}

std::shared_ptr<mga::NativeWindowReport> make_native_window_report(mo::Option const& options)
{
    if (options.is_set(fb_native_window_report_opt) &&
        options.get<std::string>(fb_native_window_report_opt) == mo::log_opt_value)
        return std::make_shared<mga::ConsoleNativeWindowReport>(std::make_shared<mir::logging::DumbConsoleLogger>());
    else
        return std::make_shared<mga::NullNativeWindowReport>();
}

mga::OverlayOptimization should_use_overlay_optimization(mo::Option const& options)
{
    if (!options.is_set(hwc_overlay_opt))
        return mga::OverlayOptimization::enabled;

    if (options.get<bool>(hwc_overlay_opt))
        return mga::OverlayOptimization::disabled;
    else
        return mga::OverlayOptimization::enabled;
}
}  // namespace

mga::Platform::Platform(std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
                        std::shared_ptr<mga::DisplayComponentFactory> const& display_buffer_builder,
                        std::shared_ptr<mg::DisplayReport> const& display_report,
                        std::shared_ptr<mga::NativeWindowReport> const& native_window_report,
                        mga::OverlayOptimization overlay_option,
                        std::shared_ptr<mga::DeviceQuirks> const& quirks)
    : buffer_allocator(buffer_allocator),
      display_buffer_builder(display_buffer_builder),
      display_report(display_report),
      quirks(quirks),
      native_window_report(native_window_report),
      overlay_option(overlay_option)
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mga::Platform::create_buffer_allocator()
{
    struct WrappingGraphicsBufferAllocator : mg::GraphicBufferAllocator
    {
        WrappingGraphicsBufferAllocator(std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
            : allocator(allocator)
        {
        }

        std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const& buffer_properties) override
        {
            return allocator->alloc_buffer(buffer_properties);
        }

        std::vector<MirPixelFormat> supported_pixel_formats() override
        {
            return allocator->supported_pixel_formats();
        }

        std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    };

    return make_module_ptr<WrappingGraphicsBufferAllocator>(buffer_allocator);
}

mir::UniqueModulePtr<mg::Display> mga::Platform::create_display(std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
                                                                std::shared_ptr<mg::GLConfig> const& gl_config)
{
    auto const program_factory = std::make_shared<mir::gl::DefaultProgramFactory>();
    return mir::make_module_ptr<mga::Display>(display_buffer_builder, program_factory, gl_config, display_report,
                                              native_window_report, overlay_option);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mga::Platform::make_ipc_operations() const
{
    return mir::make_module_ptr<mga::IpcOperations>();
}

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mir::graphics::DisplayReport> const& display_report)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    auto quirks = std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{}, *options);
    auto hwc_report = make_hwc_report(*options);
    auto overlay_option = should_use_overlay_optimization(*options);
    hwc_report->report_overlay_optimization(overlay_option);
    auto display_resource_factory = std::make_shared<mga::ResourceFactory>();

    auto component_factory = std::make_shared<mga::HalComponentFactory>(display_resource_factory, hwc_report, quirks);

    return mir::make_module_ptr<mga::Platform>(component_factory->the_buffer_allocator(), component_factory,
                                               display_report, make_native_window_report(*options), overlay_option,
                                               quirks);
}

mir::UniqueModulePtr<mg::Platform> create_guest_platform(std::shared_ptr<mg::DisplayReport> const& display_report,
                                                         std::shared_ptr<mg::NestedContext> const&)
{
    mir::assert_entry_point_signature<mg::CreateGuestPlatform>(&create_guest_platform);
    // TODO: actually allow disabling quirks for guest platform
    auto quirks = std::make_shared<mga::DeviceQuirks>(mga::PropertiesOps{});

    std::shared_ptr<mga::CommandStreamSyncFactory> sync_factory;
    if (quirks->working_egl_sync())
        sync_factory = std::make_shared<mga::EGLSyncFactory>();
    else
        sync_factory = std::make_shared<mga::NullCommandStreamSyncFactory>();

    // TODO: remove nullptr parameter once platform classes are sorted.
    //      mg::NativePlatform cannot create a display anyways, so it doesnt need a  display builder
    auto const buffer_allocator = std::make_shared<mga::GraphicBufferAllocator>(sync_factory, quirks);
    return mir::make_module_ptr<mga::Platform>(buffer_allocator, nullptr, display_report,
                                               std::make_shared<mga::NullNativeWindowReport>(),
                                               mga::OverlayOptimization::disabled, quirks);
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()(hwc_log_opt,
                         boost::program_options::value<std::string>()->default_value(std::string{mo::off_opt_value}),
                         "[platform-specific] How to handle the HWC logging report. [{log,off}]")(
        fb_native_window_report_opt,
        boost::program_options::value<std::string>()->default_value(std::string{mo::off_opt_value}),
        "[platform-specific] whether to log the EGLNativeWindowType backed by the framebuffer [{log,off}]")(
        hwc_overlay_opt,
        boost::program_options::value<bool>()->default_value(false),
        "[platform-specific] Whether to disable overlay optimizations [{on,off}]");
    mga::DeviceQuirks::add_options(config);
}

mg::PlatformPriority probe_graphics_platform(mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);
    int err;
    hw_module_t const* hw_module;

    err = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);
    if (err < 0)
        return mg::PlatformPriority::unsupported;

#ifdef ANDROID_CAF
    // LAZY HACK to check for qcom hardware
    if (force_vanilla_version())
	return mg::PlatformPriority::unsupported;
    auto version = get_android_version();
    if (force_caf_version() || 
        (strcmp(hw_module->author, "CodeAurora Forum") == 0 && std::get<0>(version) >= 7))
        return static_cast<mg::PlatformPriority>(mg::PlatformPriority::best + 1);
    return mg::PlatformPriority::unsupported;
#else
    if (force_caf_version())
	return mg::PlatformPriority::unsupported;
    return mg::PlatformPriority::best;
#endif
}

namespace
{
mir::ModuleProperties const description = {
#ifdef ANDROID_CAF
    "mir:android-caf",
#else
    "mir:android",
#endif
    MIR_VERSION_MAJOR, MIR_VERSION_MINOR, MIR_VERSION_MICRO, mir::libname()};
}  // namespace

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}
