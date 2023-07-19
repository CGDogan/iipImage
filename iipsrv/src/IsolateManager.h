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
    static std::vector<Isolate> free;

public:
    // call me with std::move
    static void free(Isolate graal_isolate)
    {
        free.push_back(std::move(graal_isolate));
    }

    static void get_new()
    {
        return new Isolate();
    }
}

#endif /* ISOLATEMANAGER_H */
