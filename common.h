#pragma once


#define _BSD_SOURCE 1
#define _POSIX_C_SOURCE 2
#define _XOPEN_SOURCE 500


#define E_COMMON 1
#define E_USAGE 2
#define E_RARE 3


#define print(x) fputs((x), stdout)

#define str_(x) #x
#define str(x) str_(x)
