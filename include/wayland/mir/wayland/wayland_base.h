/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_WAYLAND_OBJECT_H_
#define MIR_WAYLAND_OBJECT_H_

struct wl_resource;
struct wl_global;
struct wl_client;

namespace mir
{
namespace wayland
{
/**
 * An exception type representing a Wayland protocol error
 *
 * Throwing one of these from a request handler will result in the client
 * being sent a \a code error on \a source, with the printf-style \a fmt string
 * populated as the message.:
 */
class ProtocolError : public std::runtime_error
{
public:
    [[gnu::format (printf, 4, 5)]]  // Format attribute counts the hidden this parameter
    ProtocolError(wl_resource* source, uint32_t code, char const* fmt, ...);

    auto message() const -> char const*;
    auto resource() const -> wl_resource*;
    auto code() const -> uint32_t;
private:
    std::string error_message;
    wl_resource* const source;
    uint32_t const error_code;
};

/// The base class of any object that wants to provide a destroyed flag
/// The destroyed flag is only created when needed and automatically set to true on destruction
/// This pattern is only safe in a single-threaded context
class LifetimeTracker
{
public:
    LifetimeTracker() = default;
    LifetimeTracker(LifetimeTracker const&) = delete;
    LifetimeTracker& operator=(LifetimeTracker const&) = delete;

    virtual ~LifetimeTracker();
    auto destroyed_flag() const -> std::shared_ptr<bool>;

protected:
    void mark_destroyed() const;

private:
    std::shared_ptr<bool> mutable destroyed{nullptr};
};

class Resource
{
public:
    template<int V>
    struct Version
    {
    };

    Resource();
    virtual ~Resource() = default;

    Resource(Resource const&) = delete;
    Resource& operator=(Resource const&) = delete;
};

class Global
{
public:
    template<int V>
    struct Version
    {
    };

    explicit Global(wl_global* global);
    virtual ~Global();

    Global(Global const&) = delete;
    Global& operator=(Global const&) = delete;

    virtual auto interface_name() const -> char const* = 0;

    wl_global* const global;
};

void internal_error_processing_request(wl_client* client, char const* method_name);

}
}

#endif // MIR_WAYLAND_OBJECT_H_
