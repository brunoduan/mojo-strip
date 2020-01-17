#ifndef SAMPLES_COMMON_EXPORT_H_
#define SAMPLES_COMMON_EXPORT_H_
 
#if defined(COMPONENT_BUILD) && !defined(COMPILE_SAMPLES_STATICALLY)
#if defined(SAMPLES_IMPLEMENTATION)
#define SAMPLES_EXPORT __attribute__((visibility("default")))
#else
#define SAMPLES_EXPORT
#endif
 
#else // defined(COMPONENT_BUILD)
#define SAMPLES_EXPORT
#endif
 
#endif  // SAMPLES_COMMON_EXPORT_H_
