/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "launch_app.h"

#include <unistd.h>
#include <signal.h>

#include <stdexcept>
#include <cstring>


auto miral::launch_app(
    std::vector<std::string> const& app,
    mir::optional_value<std::string> const& wayland_display,
    mir::optional_value<std::string> const& mir_socket,
    mir::optional_value<std::string> const& x11_display) -> pid_t
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        {
            // POSIX.1-2001 specifies that if the disposition of SIGCHLD is set to
            // SIG_IGN or the SA_NOCLDWAIT flag is set for SIGCHLD, then children
            // that terminate do not become zombie.
            // We don't want any children to become zombies...
            struct sigaction act;
            act.sa_handler = SIG_IGN;
            sigemptyset(&act.sa_mask);
            act.sa_flags = SA_NOCLDWAIT;
            sigaction(SIGCHLD, &act, NULL);
        }

        if (mir_socket)
        {
            setenv("MIR_SOCKET", mir_socket.value().c_str(),  true);   // configure Mir socket
        }
        else
        {
            unsetenv("MIR_SOCKET");
        }

        if (wayland_display)
        {
            setenv("WAYLAND_DISPLAY", wayland_display.value().c_str(),  true);   // configure Wayland socket
        }
        else
        {
            unsetenv("WAYLAND_DISPLAY");
        }

        if (x11_display)
        {
            setenv("DISPLAY", x11_display.value().c_str(),  true);   // configure X11 socket
        }
        else
        {
            unsetenv("DISPLAY");
        }

        setenv("GDK_BACKEND", "wayland,mir", true);         // configure GTK to use Wayland (or Mir)
        setenv("QT_QPA_PLATFORM", "wayland", true);         // configure Qt to use Wayland
        unsetenv("QT_QPA_PLATFORMTHEME");                   // Discourage Qt from unsupported theme
        setenv("SDL_VIDEODRIVER", "wayland", true);         // configure SDL to use Wayland

        std::vector<char const*> exec_args;

        for (auto const& arg : app)
            exec_args.push_back(arg.c_str());

        exec_args.push_back(nullptr);

        execvp(exec_args[0], const_cast<char*const*>(exec_args.data()));

        throw std::logic_error(std::string("Failed to execute client (") + exec_args[0] + ") error: " + strerror(errno));
    }

    return pid;
}
