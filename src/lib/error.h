/*
 * error.h
 * Error enum
 * Copyright (C) Jie
 * 2025-11-14
 *
 */
#ifndef MY_ERROR_H_
#define MY_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OK,
    ErrDread,
    ErrDwrite,
    ErrAttach,
    ErrDetach,
    ErrArg,
    ErrNoMem,
    ErrBmCreate,
    ErrBmOpe,
    ErrInode,
    ErrName,
    ErrDirentExists,
    ErrInternal,
    ErrNotFound,
    ErrNoSpace,
    ErrFileFlags,
    ErrWhence,
    ErrPath
} RC; // Return Code

#ifdef __cplusplus
}
#endif
#endif
