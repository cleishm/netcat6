
#undef HAVE_SS_FAMILY

#undef HAVE___SS_FAMILY

#if defined __GLIBC__
#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 1)
#ifdef HAVE___SS_FAMILY
#define ss_family __ss_family
#endif 
#endif 
#endif 
