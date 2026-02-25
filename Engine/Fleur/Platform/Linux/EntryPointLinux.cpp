#include <Application.h>
#include <EntryPoint.h>

int FleurMain(int argc, const char* argv[])
{
    Fleur::Log::Init();

    Fleur::Application::ApplicationBootSettings settings{};
    settings.Renderer = Fleur::Graphics::EGraphicsAPI::Vulkan;
    settings.Vsync = false;
    settings.FixedDt = 0.025f;
    settings.WindowProperties.x = 100;
    settings.WindowProperties.y = 100;
    settings.WindowProperties.CanFullscreen = false;

    Fleur::Application::instance().Init(settings);
    Fleur::Application::instance().Run();

    return 0;
}
