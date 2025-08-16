#include "ServiceLocator.h"

template <>
Fleur::ServiceLocator& Fleur::singleton<Fleur::ServiceLocator>::instance()
{
    static Fleur::ServiceLocator inst;
    return inst;
}
