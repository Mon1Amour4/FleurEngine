#include <Fuego.h>

class SandboxApp : public Fuego::Application
{
public:
    SandboxApp()
    {

    }

    ~SandboxApp()
    {

    }
};

Fuego::Application* Fuego::CreateApplication()
{
    return new SandboxApp();
}
