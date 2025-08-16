#include <Application.h>
#include <EntryPoint.h>

@interface FleurApplication : NSApplication

@property Fleur::Application* application;

- (void)run;

@end

@implementation FleurApplication
;

- (void)run
{
    [[NSNotificationCenter defaultCenter] postNotificationName:NSApplicationWillFinishLaunchingNotification object:NSApp];
    [[NSNotificationCenter defaultCenter] postNotificationName:NSApplicationDidFinishLaunchingNotification object:NSApp];

    _application->Run();
}

@end

int FleurMain(int argc, const char* argv[])
{
    Fleur::Log::Init();

    FleurApplication* applicationObject = [FleurApplication alloc];

    Fleur::Application* app = Fleur::CreateApplication();
    applicationObject.application = app;

    if ([applicationObject respondsToSelector:@selector(run)])
    {
        [applicationObject performSelectorOnMainThread:@selector(run) withObject:nil waitUntilDone:YES];
    }

    delete app;

    return 0;
}
