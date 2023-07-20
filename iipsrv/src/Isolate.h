/*
 * File:   Isolate.h
 */

#ifndef ISOLATE_H
#define ISOLATE_H

#include <vector>
#include <string>
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
        fprintf(stderr, "getting receive buffer\n");

        receive_buffer = bf_get_communication_buffer(graal_thread);
        fprintf(stderr, "got.\n");

        if (bf_test(graal_thread) < 0)
        {
            fprintf(stderr, "isolate initialization got fatal error %s", bf_get_error(graal_thread));
            throw "isolate initialization got fatal error" + std::string(bf_get_error(graal_thread));
        }
        fprintf(stderr, "bf_test complete.\n");

        // Do last part of bf_test
        for (int i = 0; i < 10; i++)
        {
            if (receive_buffer[i] != i) {
                fprintf(stderr, "system's graal implementation requires querying from error buffer every time instead of using the same buffer. unimplemented: IIPSrv doesn't have this mode implemented. Please call bf_get_communication_buffer to update the pointer before every read from receive_buffer\n");
                throw "See the note in Isolate.h about unimplemented";
            }
        }
        fprintf(stderr, "buffer check complete.\n");

        bf_initialize(graal_thread);
        fprintf(stderr, "initialized.\n");


        // todo deleteme
        bf_is_compatible(graal_thread, /*(char *) path.c_str()*/ "/images/posdebug4.dcm");
            fprintf(stderr, "check1 done.\n");
    }

    // If we allow copy, the previous one might be destroyed then it'll call
    // graal_tear_down_isolate but while sharing a pointer with the new one
    // so the new one will be broken as well, so use std::move
    // https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
    // https://en.cppreference.com/w/cpp/language/rule_of_three
    // The alternative is reference counting
    // https://ps.uci.edu/~cyu/p231C/LectureNotes/lecture13:referenceCounting/lecture13.pdf
    // count, in an int*, the number of copies and deallocate when reach 0
    Isolate(const Isolate &) = delete;
    Isolate(Isolate &&) = default;
    Isolate &operator=(const Isolate &) = delete;
    Isolate &operator=(Isolate &&) = default;

    ~Isolate()
    {
        if (graal_thread)
        {
            /*bf_reset(graal_thread);
            // TODO: do tear me down
            graal_tear_down_isolate(graal_thread);*/
        }
    }
};

#endif /* ISOLATE_H */
