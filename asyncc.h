// @file asyncc.h
// Async for (embedded) C
//
// Copyright (c) 2023 Tom Wolf
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
#ifndef ASYNCC_H
#define ASYNCC_H


// TODO: investigate FOREACH() macro to implement original idea without

#define FE_0(WHAT)
#define FE_1(WHAT, X) WHAT(X) 
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)

#define GET_MACRO(_0,_1,_2,_3,_4,_5,NAME,...) NAME 
#define FOR_EACH(action,...) \
  GET_MACRO(_0,__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1,FE_0)(action,__VA_ARGS__)

// Some actions
#define L_DEFINE(X)     X;
#define L_DEFINES(...) FOR_EACH(L_DEFINE,__VA_ARGS__)

// Simplest way to provide local state is to expand it to a local struct, point
// it to the top of the stack, and advance the stack index by sizeof(struct)
#define ASYNC_LOCALS(...)       \

#define ASYNC_BEGIN(s, ...)                                         \
    uint16_t *s_idx = (uint16_t*)s;                                 \
    uint16_t *s_max = (uint16_t*)s+1;                               \
    uint16_t err = ASYNC_ERR;                                       \
    struct locals {                                                 \
        struct async *state;                                        \
        __VA_ARGS__                                                 \
    } *l = ((*s_idx + sizeof(struct locals)) < *s_max)              \
        ? ((struct locals*)((uint8_t*)s+*s_idx))                    \
        : ((struct locals*)err); switch (l->state) { default:

#endif // ASYNCC_H
