#ifndef __BFBRIDGE_H
#define __BFBRIDGE_H

#include <graal_isolate.h>


#if defined(__cplusplus)
extern "C" {
#endif

char* bf_get_error(graal_isolatethread_t*);

char bf_is_compatible(graal_isolatethread_t*, char*);

char bf_open(graal_isolatethread_t*, char*);

char bf_is_single_file(graal_isolatethread_t*, char*);

char* bf_get_used_files(graal_isolatethread_t*);

char bf_close(graal_isolatethread_t*, int);

int bf_get_resolution_count(graal_isolatethread_t*);

char bf_set_current_resolution(graal_isolatethread_t*, int);

int bf_get_size_x(graal_isolatethread_t*);

int bf_get_size_y(graal_isolatethread_t*);

int bf_get_optimal_tile_width(graal_isolatethread_t*);

int bf_get_optimal_tile_height(graal_isolatethread_t*);

char* bf_get_format(graal_isolatethread_t*);

int bf_get_pixel_type(graal_isolatethread_t*);

int bf_get_bits_per_pixel(graal_isolatethread_t*);

int bf_get_bytes_per_pixel(graal_isolatethread_t*);

int bf_get_rgb_channel_count(graal_isolatethread_t*);

char bf_is_rgb(graal_isolatethread_t*);

char bf_is_interleaved(graal_isolatethread_t*);

char bf_is_little_endian(graal_isolatethread_t*);

char bf_is_floating_point(graal_isolatethread_t*);

char bf_floating_point_is_normalized(graal_isolatethread_t*);

char* bf_get_dimension_order(graal_isolatethread_t*);

char bf_is_order_certain(graal_isolatethread_t*);

char* bf_open_bytes(graal_isolatethread_t*, int, int, int, int);

double bf_get_mpp_x(graal_isolatethread_t*);

double bf_get_mpp_y(graal_isolatethread_t*);

double bf_get_mpp_z(graal_isolatethread_t*);

char* bf_get_current_file(graal_isolatethread_t*);

char bf_is_any_file_open(graal_isolatethread_t*);

char bftools_should_generate(graal_isolatethread_t*);

char bftools_generate_subresolutions(graal_isolatethread_t*, char*, char*, int);

int run_main(int argc, char** argv);

#if defined(__cplusplus)
}
#endif
#endif
