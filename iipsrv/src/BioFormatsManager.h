/*
 * File:   BioFormatsManager.h
 */

#ifndef BIOFORMATSMANAGER_H
#define BIOFORMATSMANAGER_H

#include <vector>
#include <graal_isolate.h>
#include "BioFormatsInstance.h"
#include <libbfbridge.h>
#include <stdio.h>

class BioFormatsManager
{
private:
    static std::vector<BioFormatsInstance> free_list;

public:
    // call me with std::move
    static void free(BioFormatsInstance graal_isolate)
    {
        free_list.push_back(std::move(graal_isolate));
        bf_close(free_list.back().graal_thread, 0);
    }

    static BioFormatsInstance get_new()
    {
        if (free_list.size() == 0)
        {

            fprintf(stderr, "preparemid1\n");

            BioFormatsInstance gi;
            fprintf(stderr, "preparemid2\n");

            free_list.push_back(std::move(gi));
            fprintf(stderr, "preparemid3\n");
        }

        fprintf(stderr, "getnew start\n");
        if (free_list.size() == 0)
        {
            fprintf(stderr, "Expect crash1\n");
        }

        BioFormatsInstance gi = std::move(free_list.back());
        fprintf(stderr, "getnew mid\n");

        if (free_list.size() == 0)
        {
            fprintf(stderr, "Expect crash2\n");
        } else {
            fprintf(stderr, "not expect crash2\n");
        }
            free_list.pop_back(); // calls destructuor
            fprintf(stderr, "getnew end\n");

            return gi;
    }
};

#endif /* BIOFORMATSMANAGER_H */
