// strings.h - Main application localisation gateway
// Selects the correct language-specific string file at compile time.
// Usage: #include "strings.h"  (in src/main.c and all application sources)
// For libdemo strings, use strings_demo.h instead.

#ifndef STRINGS_H
#define STRINGS_H

#ifdef LANG_FR
#include "strings_fr.h"
#else
#include "strings_en.h"
#endif

#endif
