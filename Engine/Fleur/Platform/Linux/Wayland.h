#pragma once
#include <memory>

#include "EventQueueLinux.h"
#include "Input.h"

struct wl_display;
struct wl_compositor;
struct wl_surface;
struct wl_registry;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct xdg_toplevel;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;

namespace Wayland
{
struct Deleters
{
    void operator()(wl_display* h) const noexcept;
    void operator()(wl_compositor* h) const noexcept;
    void operator()(wl_surface* h) const noexcept;
    void operator()(wl_registry* h) const noexcept;
    void operator()(wl_seat* h) const noexcept;
    void operator()(wl_pointer* h) const noexcept;
    void operator()(wl_keyboard* h) const noexcept;
    void operator()(xdg_wm_base* h) const noexcept;
    void operator()(xdg_surface* h) const noexcept;
    void operator()(xdg_toplevel* h) const noexcept;
};

template <typename WlHandleT>
using UniquePtr = std::unique_ptr<WlHandleT, Deleters>;

struct Context
{
    bool Open(Fleur::EventQueueLinux* eq);
    bool poll();

    bool opened = false;
    bool terminate = false;
    UniquePtr<wl_display> display;
    UniquePtr<wl_compositor> compositor;
    UniquePtr<wl_registry> registry;
    UniquePtr<xdg_wm_base> xdgBase;
    UniquePtr<wl_seat> seat;
    UniquePtr<wl_pointer> pointer;
    UniquePtr<wl_keyboard> keyboard;
    Fleur::EventQueueLinux* eventQueue;

    bool focused;
    Fleur::Input::EKeyState key_states[256];
};

struct Window
{
    bool Open(std::shared_ptr<Context> ctx);
    void setTitle(const char* title_utf8);
    void flush();

    UniquePtr<wl_surface> m_surface;
    UniquePtr<xdg_surface> m_XdgSurface;
    UniquePtr<xdg_toplevel> m_XdgToplevel;

    std::shared_ptr<Context> ctx;
    uint32_t width;
    uint32_t height;
    bool closed;
};

}  // namespace Wayland
