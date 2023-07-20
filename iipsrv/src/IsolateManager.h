/*
 * File:   IsolateManager.h
 */

#ifndef ISOLATEMANAGER_H
#define ISOLATEMANAGER_H

#include <vector>
#include <graal_isolate.h>
#include "Isolate.h"
#include <libbfbridge.h>
#include <stdio.h>

class IsolateManager
{
private:
    static std::vector<Isolate> free_list;

public:
    // call me with std::move
    static void free(Isolate graal_isolate)
    {
        free_list.push_back(std::move(graal_isolate));
        bf_close(free_list.back().graal_thread, 0);
    }

    static Isolate get_new()
    {
        Isolate gi;
        if (free_list.size() != 0)
        {
            gi = std::move(free_list.back());
            free_list.pop_back();
            return std::move(gi);
        }
        else
        {
            Isolate gi;
            // todo delete me
            fprintf(stderr, "check2:.\n");

            bf_is_compatible(gi.graal_thread, /*(char *) path.c_str()*/ "/images/posdebug4.dcm");
                fprintf(stderr, "check2 done\n");

            return std::move(gi);
        }
    }
};

#endif /* ISOLATEMANAGER_H */
