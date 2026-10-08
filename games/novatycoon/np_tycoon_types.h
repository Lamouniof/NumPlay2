#include <stdint.h>
typedef uint16_t color_t;
typedef struct {uint16_t w,h,ncolors;uint32_t zsize;const color_t*palette;const uint8_t*z;} np_shot_t;
typedef struct {const char*id,*title,*tagline,*credit;uint32_t top,bottom,accent;const uint32_t*begin,*end;int(*main)(void);uint8_t*data;const uint8_t*data_init;uint32_t data_size;uint8_t*bss;uint32_t bss_size;const np_shot_t*shots;uint8_t nshots;const char*const*records;const char*const*progress;uint32_t est_size;} np_game_t;
