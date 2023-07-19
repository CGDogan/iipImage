/*
 * File:   Isolate.h
 */

#ifndef ISOLATE_H
#define ISOLATE_H

#include <vector>
#include <graal_isolate.h>
#include <libbfbridge.h>
#include <stdio.h>

class Isolate
{
public:
    graal_isolatethread_t *graal_thread = NULL;
    char *receive_buffer = NULL;

    Isolate()
    {
        graal_isolate_t *graal_isolate;
        fprintf(stderr, "dddBioFormatsImage.h3: Creating isolate\n");
        int code = graal_create_isolate(NULL, &graal_isolate, &graal_thread);
        fprintf(stderr, "dddBioFormatsImage.h3: Created isolate. should be 0: %d\n", code);
        if (code != 0)
        {
            fprintf(stderr, "dddBioFormatsImage.h3: ERROR But with error!\n");
            throw "graal_create_isolate: " + code;
        }

        receive_buffer = bf_get_communication_buffer(graal_thread);

        if (bf_test(graal_thread) < 0)
        {
            fprintf(stderr, "isolate initialization got fatal error %s", bf_get_error(graal_thread));
            throw "isolate initialization got fatal error" + std::to_string(bf_get_error(graal_thread));
        }

        // Do last part of bf_test
        for (int i = 0; i < 10; i++)
        {
            if (receive_buffer[i] != i) {
                fprintf(stderr, "system's graal implementation requires querying from error buffer every time instead of using the same buffer. unimplemented: IIPSrv doesn't have this mode implemented. Please call bf_get_communication_buffer to update the pointer before every read from receive_buffer\n");
                throw "See the note in Isolate.h about unimplemented";
            }
        }

        bf_initialize(graal_thread);
    }

    ~Isolate()
    {
        if (graal_thread)
        {
            bf_reset(graal_thread);
            graal_tear_down_isolate(graal_thread);
        }
    }
}

#endif /* ISOLATE_H */
