#ifndef LP_VERSION_H
#define LP_VERSION_H

#define LP_STR_HELPER(x) #x
#define LP_STR(x) LP_STR_HELPER(x)

// __LP__
#ifndef __LP__
#define LP_MAJOR_VER  1
#define LP_MINOR_VER  1
#define LP_PATCHLEVEL 0
#define LP_BUILD "devel"

#define LP_VERSION LP_MAJOR_VER * 10000 \
    + LP_MINOR_VER * 100 \
    + LP_PATCHLEVEL

#define __LP__ \
    LP_STR(LP_MAJOR_VER) \
    "." \
    LP_STR(LP_MINOR_VER) \
    "." \
    LP_STR(LP_PATCHLEVEL)

#define LP_VERSION_STR_HELPER(build) \
   build

#define LP_VERSION_STR \
    LP_VERSION_STR_HELPER(__LP__)

#endif // __LP__

#endif // LP_VERSION_H

