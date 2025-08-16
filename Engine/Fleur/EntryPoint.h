#pragma once

#if defined(FLEUR_PLATFORM_MACOS)
int FleurMain(int argc, const char* argv[]);
#elif defined(FLEUR_PLATFORM_WIN)
int APIENTRY FleurMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow);
#endif
