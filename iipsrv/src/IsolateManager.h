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

    // This cannot be merged with get_new, due to return value opt. restrictions
    static void prepare()
    {
        fprintf(stderr, "prepare start\n");

        if (free_list.size() == 0) {

            fprintf(stderr, "preparemid1\n");

            Isolate gi;
            fprintf(stderr, "preparemid2\n");

            free_list.push_back(std::move(gi));
            fprintf(stderr, "preparemid3\n");
        }
        fprintf(stderr, "prepare end\n");
    }

    static Isolate get_new()
    {
        fprintf(stderr, "getnew start\n");
        Isolate gi = std::move(free_list.back());
        fprintf(stderr, "getnew mid\n");

        free_list.pop_back();
        fprintf(stderr, "getnew end\n");

        return gi;
    }
};

#endif /* ISOLATEMANAGER_H */
