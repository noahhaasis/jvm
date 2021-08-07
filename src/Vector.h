#ifndef VECTOR_H
#define VECTOR_H

#include "common.h"
#include <stdlib.h>
#include <assert.h>

template<typename T>
class Vector {
public:
  Vector(u32 initial_capacity) {
    m_size = 0;
    m_capacity = initial_capacity;
    m_data = (T *)malloc(initial_capacity * sizeof(T));
  }

  Vector(void) {
    m_size = 0;
    m_capacity = VECTOR_INITIAL_CAPACITY;
    m_data = (T *)malloc(VECTOR_INITIAL_CAPACITY * sizeof(T));
  }

  T get(u32 index) {
    assert(index < m_size);
    return m_data[index];
  }

  void push(T value) {
    if(m_size == m_capacity) {
      resize();
    }

    m_data[m_size++] = value;
  }

  T pop(void) {
    return m_data[--m_size];
  }

  bool empty(void) {
    return m_size == 0;
  }

  u32 length(void) {
    return m_size;
  }

private:
  u32 m_size;
  u32 m_capacity;
  T  *m_data;

  void resize() {
    m_capacity *= 2;
    m_data = (T *)realloc(m_data, m_capacity*sizeof(T));
  }

  static u32 constexpr VECTOR_INITIAL_CAPACITY = 8;
};

#endif
