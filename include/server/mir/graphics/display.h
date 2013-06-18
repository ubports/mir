/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_DISPLAY_H_
#define MIR_GRAPHICS_DISPLAY_H_

#include "mir/graphics/viewable_area.h"

#include <memory>
#include <functional>

namespace mir
{

class MainLoop;

namespace graphics
{

class DisplayBuffer;
class DisplayConfiguration;
class Cursor;
class GLContext;

typedef std::function<bool()> DisplayPauseHandler;
typedef std::function<bool()> DisplayResumeHandler;

/**
 * Interface to the display subsystem.
 */
class Display : public ViewableArea
{
public:
    /**
     * The display's view area.
     */
    virtual geometry::Rectangle view_area() const = 0;

    /**
     * Executes a functor for each output framebuffer.
     */
    virtual void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f) = 0;

    /**
     * Gets the current output configuration.
     */
    virtual std::shared_ptr<DisplayConfiguration> configuration() = 0;

    /**
     * Registers handlers for pausing and resuming the display subsystem.
     *
     * The implementation should use the functionality provided by the MainLoop
     * to register the handlers in a way appropriate for the platform.
     */
    virtual void register_pause_resume_handlers(
        MainLoop& main_loop,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) = 0;

    /**
     * Pauses the display.
     *
     * This method may temporarily (until resumed) release any resources
     * associated with the display subsystem.
     */
    virtual void pause() = 0;

    /**
     * Resumes the display.
     */
    virtual void resume() = 0;

    /**
     * Gets the hardware cursor object.
     */
    virtual std::weak_ptr<Cursor> the_cursor() = 0;

    /**
     * Creates a GLContext object that shares resources with the Display's GL context.
     *
     * This is usually implemented as a shared EGL context. This object can be used
     * to access graphics resources from an arbitrary thread.
     */
    virtual std::unique_ptr<GLContext> create_gl_context() = 0;

protected:
    Display() = default;
    ~Display() = default;
private:
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_DISPLAY_H_ */
