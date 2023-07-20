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
        if (free_list.size() == 0) {
            Isolate gi;

            free_list.push_back(std::move(gi));
        }
        Isolate gi;

        gi = std::move(free_list.back());
        free_list.pop_back();
        return gi;
    }
};

#endif /* ISOLATEMANAGER_H */
