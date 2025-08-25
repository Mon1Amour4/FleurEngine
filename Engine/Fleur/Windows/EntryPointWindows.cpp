#include <Application.h>
#include <EntryPoint.h>

int APIENTRY FleurMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    UNUSED(hInst);
    UNUSED(hInstPrev);
    UNUSED(cmdline);
    UNUSED(cmdshow);

    AllocConsole();
    FILE* stream;
    assert(freopen_s(&stream, "CONOUT$", "w", stdout) == 0);
    assert(freopen_s(&stream, "CONOUT$", "w", stderr) == 0);

    Fleur::Log::Init();

    Fleur::Application::ApplicationBootSettings settings{};
    settings.Renderer = Fleur::Graphics::GraphicsAPI::OpenGL;
    settings.Vsync = false;
    settings.FixedDt = 0.025f;
    settings.WindowProperties.x = 100;
    settings.WindowProperties.y = 100;
    settings.WindowProperties.CanFullscreen = false;

    Fleur::Application::instance().Init(settings);
    Fleur::Application::instance().Run();

    FreeConsole();

    return 0;
}
