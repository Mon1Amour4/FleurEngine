#include "Wayland.h"

extern "C"
{
#include <poll.h>
#include <wayland-client.h>

// $wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.xml.h
#include "wayland/xdg-shell.xml.h"
// $wayland-scanner private-code  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.xml.c
#include "wayland/xdg-shell.xml.c"
}

#include "KeyCodesLinux.h"

using Wayland::Context;
using Wayland::UniquePtr;
using Wayland::Window;

template <typename T>
struct WlArrayIterator
{
    WlArrayIterator(wl_array* array)
    {
        b = static_cast<T*>(array->data);
        e = reinterpret_cast<T*>(static_cast<uint8_t*>(array->data) + array->size);
    }

    T* begin() const
    {
        return b;
    }

    T* end() const
    {
        return e;
    }


    T* b;
    T* e;
};

void Wayland::Deleters::operator()(wl_display* h) const noexcept
{
    if (!h)
        return;
    wl_display_disconnect(h);
}

void Wayland::Deleters::operator()(wl_compositor* h) const noexcept
{
    if (!h)
        return;
    wl_compositor_destroy(h);
}

void Wayland::Deleters::operator()(wl_surface* h) const noexcept
{
    if (!h)
        return;
    wl_surface_destroy(h);
}

void Wayland::Deleters::operator()(wl_registry* h) const noexcept
{
    if (!h)
        return;
    wl_registry_destroy(h);
}

void Wayland::Deleters::operator()(wl_seat* h) const noexcept
{
    if (!h)
        return;
    wl_seat_release(h);
}

void Wayland::Deleters::operator()(wl_pointer* h) const noexcept
{
    if (!h)
        return;
    wl_pointer_release(h);
}

void Wayland::Deleters::operator()(wl_keyboard* h) const noexcept
{
    if (!h)
        return;
    wl_keyboard_release(h);
}

void Wayland::Deleters::operator()(xdg_wm_base* h) const noexcept
{
    if (!h)
        return;
    xdg_wm_base_destroy(h);
}

void Wayland::Deleters::operator()(xdg_surface* h) const noexcept
{
    if (!h)
        return;
    xdg_surface_destroy(h);
}

void Wayland::Deleters::operator()(xdg_toplevel* h) const noexcept
{
    if (!h)
        return;
    xdg_toplevel_destroy(h);
}

template <typename T>
UniquePtr<T> ToUptr(void* h)
{
    return UniquePtr<T>(static_cast<T*>(h));
}

struct XdgBaseListener
{
    static void ping(void*, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
    {
        xdg_wm_base_pong(xdg_wm_base, serial);
    }

    constexpr static const xdg_wm_base_listener Instance = {ping};
};

struct KeyboardListener
{
    static void keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size)
    {
    }

    static void enter(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
    {
        auto ctx = static_cast<Context*>(data);
        ctx->focused = true;
        for (auto key : WlArrayIterator<uint32_t>(keys))
        {
            auto fkey = GetKeyCodeLinuxEvdev(key);
            if (fkey == Fleur::Key::Unknown)
                continue;
            ctx->key_states[fkey] = Fleur::Input::KEY_PRESSED;
        }
    }

    static void leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
    {
        // Window with 'surface' lost focus
        auto ctx = static_cast<Context*>(data);
        ctx->focused = false;
        for (auto& key_state : ctx->key_states) key_state = Fleur::Input::KEY_NONE;
    }

    static void key(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
    {
        auto ctx = static_cast<Context*>(data);
        auto fkey = GetKeyCodeLinuxEvdev(key);
        if (fkey == Fleur::Key::Unknown)
            return;
        auto fkey_state = &ctx->key_states[fkey];

        switch (wl_keyboard_key_state(state))
        {
        case WL_KEYBOARD_KEY_STATE_PRESSED:
        {
            *fkey_state = Fleur::Input::KEY_PRESSED;
            ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::KeyPressedEvent(fkey, 1)));
            break;
        }
        case WL_KEYBOARD_KEY_STATE_RELEASED:
        {
            *fkey_state = Fleur::Input::KEY_RELEASED;
            ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::KeyReleasedEvent(fkey)));
            break;
        }
        case WL_KEYBOARD_KEY_STATE_REPEATED:
            *fkey_state = Fleur::Input::KEY_REPEAT;
            break;
        }
    }

    static void modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
    {
    }

    static void repeat_info(void*, struct wl_keyboard*, int32_t, int32_t)
    {
    }

    constexpr static const struct wl_keyboard_listener Instance = {keymap, enter, leave, key, modifiers, repeat_info};
};

// TODO mouse events
struct PointerListener
{
    static void enter(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
    {
    }
    static void leave(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface)
    {
    }
    static void motion(void* data, struct wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
    {
    }
    static void button(void* data, struct wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
    {
    }
    static void axis(void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
    {
    }
    static void frame(void* data, struct wl_pointer* wl_pointer)
    {
    }
    static void axis_source(void* data, struct wl_pointer* wl_pointer, uint32_t axis_source)
    {
    }
    static void axis_stop(void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis)
    {
    }
    static void axis_discrete(void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t discrete)
    {
    }
    static void axis_value120(void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t value120)
    {
    }
    static void axis_relative_direction(void* data, struct wl_pointer* wl_pointer, uint32_t axis, uint32_t direction)
    {
    }

    constexpr static const wl_pointer_listener Instance = {
        enter, leave, motion, button, axis, frame, axis_source, axis_stop, axis_discrete, axis_value120, axis_relative_direction};
};

struct SeatListener
{
    static void capabilities(void* data, struct wl_seat* wl_seat, uint32_t capabilities)
    {
        auto ctx = static_cast<Context*>(data);

        bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

        if (have_keyboard and !ctx->keyboard)
        {
            ctx->keyboard = ToUptr<wl_keyboard>(wl_seat_get_keyboard(ctx->seat.get()));
            if (ctx->keyboard)
            {
                if (wl_keyboard_add_listener(ctx->keyboard.get(), &KeyboardListener::Instance, ctx) == -1)
                    ctx->keyboard = nullptr;
            }
        }
        else if (!have_keyboard and ctx->keyboard.get())
        {
            ctx->keyboard = nullptr;
        }

        bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

        if (have_pointer and !ctx->pointer)
        {
            ctx->pointer = ToUptr<wl_pointer>(wl_seat_get_pointer(ctx->seat.get()));
            if (wl_pointer_add_listener(ctx->pointer.get(), &PointerListener::Instance, ctx) == -1)
                ctx->pointer = nullptr;
        }
        else if (!have_pointer and ctx->pointer)
        {
            ctx->pointer = nullptr;
        }
    }

    static void name(void* data, struct wl_seat* wl_seat, const char* name)
    {
    }

    constexpr static const wl_seat_listener Instance = {capabilities, name};
};

struct XdgSurfaceListener
{
    static void surface_configure(void*, struct xdg_surface* xdg_surface, uint32_t serial)
    {
        xdg_surface_ack_configure(xdg_surface, serial);
    }

    constexpr static const xdg_surface_listener Instance = {surface_configure};
};

struct XdgTopLevelListener
{
    static void toplevel_configure(void* data, struct xdg_toplevel*, int32_t width, int32_t height, struct wl_array*)
    {
        auto window = reinterpret_cast<Window*>(data);

        uint32_t new_width = width > 0 ? uint32_t(width) : window->width;
        uint32_t new_height = height > 0 ? uint32_t(height) : window->height;
        if (new_width != window->width or new_height != window->height)
        {
            auto ctx = window->ctx.get();
            ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::WindowResizeEvent(0, 0, new_width, new_height)));
            ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::WindowStartResizeEvent(0, 0, new_width, new_height, 0, 0)));
            ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::WindowEndResizeEvent(0, 0, new_width, new_height)));
            window->width = new_width;
            window->height = new_height;
        }
    }

    static void toplevel_close(void* data, struct xdg_toplevel*)
    {
        auto window = reinterpret_cast<Window*>(data);
        window->closed = true;
        auto ctx = window->ctx.get();
        ctx->eventQueue->PushEvent(std::make_shared<Fleur::EventVariant>(Fleur::WindowCloseEvent()));
    }

    constexpr static const xdg_toplevel_listener Instance = {toplevel_configure, toplevel_close};
};

struct RegistryListener
{
    static void global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t)
    {
        auto ctx = static_cast<Context*>(data);
        if (!ctx->compositor and strcmp(interface, wl_compositor_interface.name) == 0)
        {
            ctx->compositor = ToUptr<wl_compositor>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
        }
        else if (!ctx->xdgBase and strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            ctx->xdgBase = ToUptr<xdg_wm_base>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
            if (ctx->xdgBase and xdg_wm_base_add_listener(ctx->xdgBase.get(), &XdgBaseListener::Instance, NULL) == -1)
            {
                perror("failed to add xdg_wm_base listener");
                ctx->xdgBase = nullptr;
            };
        }
        else if (!ctx->seat and strcmp(interface, wl_seat_interface.name) == 0)
        {
            ctx->seat = ToUptr<wl_seat>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
            if (ctx->seat and wl_seat_add_listener(ctx->seat.get(), &SeatListener::Instance, ctx) == -1)
            {
                perror("failed to add wl_seat listener");
                ctx->seat = nullptr;
            }
        }
    };

    static void global_remove(void* data, wl_registry*, uint32_t)
    {
        auto ctx = static_cast<Context*>(data);
        ctx->terminate = true;
    }

    constexpr static const wl_registry_listener Instance = {RegistryListener::global, RegistryListener::global_remove};
};

bool Context::Open(Fleur::EventQueueLinux* eq)
{
    if (opened)
        return false;

    for (auto& key : key_states) key = {};
    focused = false;

    eventQueue = eq;

    opened = true;
    terminate = true;

    display = UniquePtr<wl_display>(wl_display_connect(nullptr));
    if (display)
        registry = UniquePtr<wl_registry>(wl_display_get_registry(display.get()));

    if (!registry)
    {
        perror("failed to connect to wayland registry");
        return false;
    }

    if (wl_registry_add_listener(registry.get(), &RegistryListener::Instance, this) == -1)
    {
        perror("failed to add wayland global listener");
        return false;
    }

    if (wl_display_roundtrip(display.get()) == -1)
    {
        perror("wayland connection error");
        return false;
    }

    if (!compositor.get() or !xdgBase.get() or !seat.get())
        return false;

    terminate = false;
    return true;
}

bool Context::poll()
{
    if (!display)
        return false;

    int fd = wl_display_get_fd(display.get());
    if (wl_display_dispatch_pending(display.get()) == -1)
        return false;

    if (wl_display_prepare_read(display.get()) != 0)
    {
        if (wl_display_dispatch_pending(display.get()) == -1)
            return false;
        return true;
    }

    struct pollfd pfd = {fd, POLLIN, 0};
    if (::poll(&pfd, 1, 0) <= 0)
    {
        wl_display_cancel_read(display.get());
        return true;
    }

    int ret = wl_display_read_events(display.get());
    if (ret != 0)
    {
        if (errno == EAGAIN)
        {
            wl_display_cancel_read(display.get());
            return true;
        }

        perror("Wayland read_events failed");
        return false;
    }

    if (wl_display_dispatch_pending(display.get()) == -1)
        return false;

    return true;
}

bool Window::Open(std::shared_ptr<Context> in_ctx)
{
    ctx = in_ctx;

    if (ctx->compositor and ctx->xdgBase)
    {
        m_surface = UniquePtr<wl_surface>(wl_compositor_create_surface(ctx->compositor.get()));
    }

    if (m_surface)
    {
        m_XdgSurface = UniquePtr<struct xdg_surface>(static_cast<struct xdg_surface*>(xdg_wm_base_get_xdg_surface(ctx->xdgBase.get(), m_surface.get())));
    }

    if (m_XdgSurface)
    {
        if (xdg_surface_add_listener(m_XdgSurface.get(), &XdgSurfaceListener::Instance, NULL) == -1)
            return false;
    }

    if (m_XdgSurface)
    {
        m_XdgToplevel = UniquePtr<struct xdg_toplevel>(static_cast<struct xdg_toplevel*>(xdg_surface_get_toplevel(m_XdgSurface.get())));
    }

    if (m_XdgToplevel)
    {
        if (xdg_toplevel_add_listener(m_XdgToplevel.get(), &XdgTopLevelListener::Instance, this) == -1)
            return false;
    }

    flush();

    return m_XdgToplevel and wl_display_roundtrip(ctx->display.get()) != -1;
}

void Window::setTitle(const char* titleUtf8)
{
    if (m_XdgToplevel)
        xdg_toplevel_set_title(m_XdgToplevel.get(), titleUtf8);
}

void Window::flush()
{
    if (m_surface)
        wl_surface_commit(m_surface.get());
}
