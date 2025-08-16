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
    settings.renderer = Fleur::Graphics::GraphicsAPI::OpenGL;
    settings.vsync = false;
    settings.fixed_dt = 0.025f;
    settings.window_props.x = 100;
    settings.window_props.y = 100;
    settings.window_props.CanFullscreen = false;

    Fleur::Application::instance().Init(settings);
    Fleur::Application::instance().Run();

    FreeConsole();

    return 0;
}
