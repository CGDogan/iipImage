/*
 * File:   BioFormatsInstance.h
 */

#ifndef BIOFORMATSINSTANCE_H
#define BIOFORMATSINSTANCE_H

#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <jni.h>
#include "BioFormatsThread.h"

/*
Memory management
helpful webpages https://stackoverflow.com/questions/2093112
https://stackoverflow.com/questions/10617735
Especially https://stackoverflow.com/a/13940735
Receiving objects gives us local references, which would normally be freed
when we return from the current function, but we don't return to Java,
C++ is our main code, so for us there's no difference between local references
and global references. Global references are normally the ones that won't be freed
on return. Nevertheless use global references for stylistic correctness.

For simplicity don't use jstring but use our common communication_buffer.

To use this library, do:
BioFormatsInstance bfi = BioFormatsManager::get_new();
bfi.bf_open(filename); // or any other methods
...
and when you're done:
BioFormatsManager::free(std::move(bfi));

Or see BioFormatsImage.cc/h for an example of keeping the Java class alive with
the C++ class
*/

// Allow 2048*2048 four channels of 16 bits
#define bfi_communication_buffer_len 33554432

class BioFormatsInstance
{
public:
  // std::unique_ptr<BioFormatsThread> or shared ptr would also work
  static BioFormatsThread thread;

  bfbridge_instance_t bfinstance;

  BioFormatsInstance();

  // If we allow copy, the previous one might be destroyed then it'll call
  // destroy VM but while sharing a pointer with the new one
  // so the new one will be broken as well, so use std::move
  // https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
  // https://en.cppreference.com/w/cpp/language/rule_of_three
  // The alternative is reference counting
  // https://ps.uci.edu/~cyu/p231C/LectureNotes/lecture13:referenceCounting/lecture13.pdf
  // Or the simplest way would be to use a pointer to BioFormatsInstance.
  // Pointers are easier to move manually and count, in an int*, the number of copies and deallocate when reach 0

  // We can't use default ones because we want to avoid leaking classes.
  // jclass must be tracked until its destruction and freed manually.
  // When we move-construct or move-assign the previous references should be destroyed,
  // otherwise the previous one's destructor will free it so the new
  // one will be broken. That's why we can't use defaults instead of Rule of 5.
  // Here we choose to reuse jclasses rather than to destroy
  // and recreate them unnecessarily.
  BioFormatsInstance(const BioFormatsInstance &) = delete;
  BioFormatsInstance(BioFormatsInstance &&other)
  {
    // Copy both the Java instance class and the communication buffer
    // Moving removes the java class pointer from the previous
    // so that the destruction of it doesn't break the newer class
    bfbridge_move_instance(&bfinstance, &other.bfinstance);
  }
  BioFormatsInstance &operator=(const BioFormatsInstance &) = delete;
  BioFormatsInstance &operator=(BioFormatsInstance &&other)
  {
    bfbridge_move_instance(&bfinstance, &other.bfinstance);
    return *this;
  }

  char *communication_buffer()
  {
    return bfbridge_instance_get_communication_buffer(&bfinstance, NULL);
  }

  ~BioFormatsInstance()
  {
    char *buffer = communication_buffer();
    if (buffer)
    {
      delete[] buffer;
    }

    bfbridge_free_instance(&bfinstance, &thread.bflibrary);
  }

  // changed ownership: user opened new file, etc.
  void refresh()
  {
    fprintf(stderr, "calling refresh\n");
    // Here is an example of calling a method manually without the C wrapper
    thread.bflibrary.env->CallVoidMethod(bfinstance.bfbridge, thread.bflibrary.BFClose);
    fprintf(stderr, "called refresh\n");
  }

  // To be called only just after a function returns an error code
  std::string get_error()
  {
    std::string err;
    char *buffer = communication_buffer();
    err.assign(communication_buffer,
               bf_get_error_length(bfinstance.bfbridge, thread.bflibrary));
    return err;
  }

  int is_compatible(std::string filepath)
  {
    return bf_is_compatible(bfinstance.bfbridge, thread.bflibrary, &filepath[0], len);
  }

  int open(std::string filepath)
  {
    return bf_open(bfinstance.bfbridge, thread.bflibrary, &filepath[0], len);
  }

  int close()
  {
    return bf_close(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_resolution_count()
  {
    return bf_get_resolution_count(bfinstance.bfbridge, thread.bflibrary);
  }

  int set_current_resolution(int res)
  {
    return bf_set_current_resolution(bfinstance.bfbridge, thread.bflibrary, res);
  }

  int get_size_x()
  {
    return bf_get_size_x(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_size_y()
  {
    return bf_get_size_y(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_size_z()
  {
    return bf_get_size_z(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_size_c()
  {
    return bf_get_size_c(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_size_t()
  {
    return bf_get_size_t(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_effective_size_c()
  {
    return bf_get_size_c(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_optimal_tile_width()
  {
    return bf_get_optimal_tile_width(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_optimal_tile_height()
  {
    return bf_get_optimal_tile_height(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_pixel_type()
  {
    return bf_get_pixel_type(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_bytes_per_pixel()
  {
    return bf_get_bytes_per_pixel(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_rgb_channel_count()
  {
    return bf_get_rgb_channel_count(bfinstance.bfbridge, thread.bflibrary);
  }

  int get_image_count()
  {
    return bf_get_rgb_channel_count(bfinstance.bfbridge, thread.bflibrary);
  }

  int is_rgb()
  {
    return bf_is_rgb(bfinstance.bfbridge, thread.bflibrary);
  }

  int is_interleaved()
  {
    return bf_is_interleaved(bfinstance.bfbridge, thread.bflibrary);
  }

  int is_little_endian()
  {
    return bf_is_little_endian(bfinstance.bfbridge, thread.bflibrary);
  }

  int is_false_color()
  {
    return bf_is_false_color(bfinstance.bfbridge, thread.bflibrary);
  }

  int is_indexed_color()
  {
    return bf_is_indexed_color(bfinstance.bfbridge, thread.bflibrary);
  }

  std::string get_dimension_order()
  {
    int len = bf_get_dimension_order(bfinstance.bfbridge, thread.bflibrary);
    if (len < 0)
    {
      return "";
    }
    std::string s;
    char *buffer = communication_buffer();
    s.assign(buffer, len);
    return s;
  }

  int is_order_certain()
  {
    return bf_is_order_certain(bfinstance.bfbridge, thread.bflibrary);
  }

  int open_bytes(int x, int y, int w, int h)
  {
    return bf_open_bytes(bfinstance.bfbridge, thread.bflibrary, 0, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */
