/*
 * error.h
 * Error enum
 * Copyright (C) Jie
 * 2025-11-14
 *
 */
#ifndef MY_ERROR_H_
#define MY_ERROR_H_

typedef enum {
    OK,
    ErrDread,
    ErrDwrite,
    ErrAttach,
    ErrDetach
} RC; // Return Code

#endif
