// strings_demo.h - libdemo localisation gateway
// Selects the correct language-specific string file at compile time.
// Usage: #include "strings_demo.h"  (in src/libdemo.c only)

#ifndef STRINGS_DEMO_H
#define STRINGS_DEMO_H

#ifdef LANG_FR
#include "strings_demo_fr.h"
#else
#include "strings_demo_en.h"
#endif

#endif
