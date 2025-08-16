﻿#pragma once

#include <variant>

#include "Events/ApplicationEvent.h"
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

namespace Fleur
{
using EventVariant =
    std::variant<WindowResizeEvent, WindowCloseEvent, AppTickEvent, AppUpdateEvent, AppRenderEvent, KeyPressedEvent, KeyReleasedEvent, MouseMovedEvent,
                 MouseScrolledEvent, MouseButtonPressedEvent, MouseButtonReleasedEvent, WindowValidateEvent, WindowStartResizeEvent, WindowEndResizeEvent>;

template <class... Ts>
struct EventVisitor : Ts...
{
    using Ts::operator()...;
};

// Deduction guide
template <class... Ts>
EventVisitor(Ts...) -> EventVisitor<Ts...>;

}  // namespace Fleur
