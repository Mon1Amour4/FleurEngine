#include "ServiceLocator.h"

template <>
Fuego::ServiceLocator& Fuego::singleton<Fuego::ServiceLocator>::instance()
{
    static Fuego::ServiceLocator inst;
    return inst;
}
