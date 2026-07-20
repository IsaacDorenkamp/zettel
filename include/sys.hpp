#pragma once

#include <unistd.h>

namespace zettel {

namespace sys {

int spawn(const char* executable, char* const argv[], char* const env[]);

}

}
