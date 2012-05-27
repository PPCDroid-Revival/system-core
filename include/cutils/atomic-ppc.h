/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_CUTILS_ATOMIC_PPC_H
#define ANDROID_CUTILS_ATOMIC_PPC_H

#include <stdint.h>

extern inline void android_compiler_barrier(void)
{
    __asm__ __volatile__ ("" : : : "memory");
}

//#if ANDROID_SMP == 0
#if 0
extern inline void android_memory_barrier(void)
{
    android_compiler_barrier();
}
extern inline void android_memory_store_barrier(void)
{
    android_compiler_barrier();
}
#else
extern inline void android_memory_barrier(void)
{
    __asm__ __volatile__ ("msync" : : : "memory");
}
extern inline void android_memory_store_barrier(void)
{
//    android_compiler_barrier();
    android_memory_barrier();
}
#endif

extern inline int32_t android_atomic_acquire_load(volatile const int32_t *ptr)
{
    int32_t value = *ptr;
    android_compiler_barrier();
    return value;
}

extern inline int32_t android_atomic_release_load(volatile const int32_t *ptr)
{
    android_memory_barrier();
    return *ptr;
}

extern inline void android_atomic_acquire_store(int32_t value,
                                                volatile int32_t *ptr)
{
    *ptr = value;
    android_memory_barrier();
}

extern inline void android_atomic_release_store(int32_t value,
                                                volatile int32_t *ptr)
{
    android_memory_barrier();
    *ptr = value;
}

extern inline int android_atomic_cas(int32_t old_value, int32_t new_value,
                                     volatile int32_t *ptr)
{
    int32_t prev;
    __asm__ __volatile__ ("\n1:"
            "\tlwarx %0,0,%3\n"
            "\tcmpw %0,%4\n" 
            "\tbne- 2f\n"
            "\tstwcx. %2,0,%3\n"
            "\tbne- 1b\n"
            "2:\n"
                          : "=&r" (prev), "+m" (*ptr)
                          : "r" (new_value), "r" (ptr), "r" (old_value)
                          : "memory", "cc");
    return prev != old_value;
}

extern inline int android_atomic_acquire_cas(int32_t old_value,
                                             int32_t new_value,
                                             volatile int32_t *ptr)
{
    int status = android_atomic_cas(old_value, new_value, ptr);
    android_memory_barrier();
    return status;
}

extern inline int android_atomic_release_cas(int32_t old_value,
                                             int32_t new_value,
                                             volatile int32_t *ptr)
{
    android_memory_barrier();
    return android_atomic_cas(old_value, new_value, ptr);
}

extern inline int32_t android_atomic_swap(int32_t new_value,
                                     volatile int32_t *ptr)
{
    int32_t prev;
    __asm__ __volatile__ ("\n1:"
            "\tlwarx %0,0,%3\n"
            "\tstwcx. %2,0,%3\n"
            "\tbne- 1b\n"
                          : "=&r" (prev), "+m" (*ptr)
                          : "r" (new_value), "r" (ptr)
                          : "memory", "cc");
    return prev;
}

extern inline int32_t android_atomic_add(int32_t increment,
                                         volatile int32_t *ptr)
{
    int32_t prev;
    __asm__ __volatile__ ("\n1:"
                "\tlwarx %0,0,%3\n"
                "\tadd %2,%2,%0\n"
                "\tstwcx. %2,0,%3\n"
                "\tbne- 1b\n"
                : "=&r" (prev), "+m" (*ptr), "+r" (increment)
                : "r" (ptr)
                : "memory", "cc");
    /* prev now holds the old value of *ptr */
    return prev;
}

extern inline int32_t android_atomic_inc(volatile int32_t *addr)
{
    return android_atomic_add(1, addr);
}

extern inline int32_t android_atomic_dec(volatile int32_t *addr)
{
    return android_atomic_add(-1, addr);
}

extern inline int32_t android_atomic_and(int32_t value,
                                         volatile int32_t *ptr)
{
    int32_t prev;
    __asm__ __volatile__ ("\n1:"
                "\tlwarx %0,0,%3\n"
                "\tand %2,%2,%0\n"
                "\tstwcx. %2,0,%3\n"
                "\tbne- 1b\n"
                : "=&r" (prev), "+m" (*ptr), "+r" (value)
                : "r" (ptr)
                : "memory", "cc");
    /* prev now holds the old value of *ptr */
    return prev;
}

extern inline int32_t android_atomic_or(int32_t value, volatile int32_t *ptr)
{
    int32_t prev;
    __asm__ __volatile__ ("\n1:"
                "\tlwarx %0,0,%3\n"
                "\tor %2,%2,%0\n"
                "\tstwcx. %2,0,%3\n"
                "\tbne- 1b\n"
                : "=&r" (prev), "+m" (*ptr), "+r" (value)
                : "r" (ptr)
                : "memory", "cc");
    /* prev now holds the old value of *ptr */
    return prev;
}

#endif /* ANDROID_CUTILS_ATOMIC_PPC_H */
