#ifndef __BFBRIDGE_H
#define __BFBRIDGE_H

#include <graal_isolate_dynamic.h>


#if defined(__cplusplus)
extern "C" {
#endif

typedef char* (*bf_get_error_fn_t)(graal_isolatethread_t*);

typedef char (*bf_open_fn_t)(graal_isolatethread_t*, char*);

typedef char (*bf_is_single_file_fn_t)(graal_isolatethread_t*, char*);

typedef char (*bf_close_fn_t)(graal_isolatethread_t*, int);

typedef int (*bf_get_resolution_count_fn_t)(graal_isolatethread_t*);

typedef char (*bf_set_resolution_count_fn_t)(graal_isolatethread_t*, int);

typedef int (*bf_get_size_x_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_size_y_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_optimal_tile_Width_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_optimal_tile_height_fn_t)(graal_isolatethread_t*);

typedef char* (*bf_get_format_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_pixel_type_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_bits_per_pixel_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_bytes_per_pixel_fn_t)(graal_isolatethread_t*);

typedef int (*bf_get_rgb_channel_count_fn_t)(graal_isolatethread_t*);

typedef char (*bf_is_rgb_fn_t)(graal_isolatethread_t*);

typedef char (*bf_is_interleaved_fn_t)(graal_isolatethread_t*);

typedef char (*bf_is_little_endian_fn_t)(graal_isolatethread_t*);

typedef char (*bf_is_floating_point_fn_t)(graal_isolatethread_t*);

typedef char* (*bf_get_dimension_order_fn_t)(graal_isolatethread_t*);

typedef double (*bf_get_mpp_x_fn_t)(graal_isolatethread_t*);

typedef double (*bf_get_mpp_y_fn_t)(graal_isolatethread_t*);

typedef double (*bf_get_mpp_z_fn_t)(graal_isolatethread_t*);

typedef int (*run_main_fn_t)(int argc, char** argv);

#if defined(__cplusplus)
}
#endif
#endif
