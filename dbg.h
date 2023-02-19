#ifndef __DBG_H__
#define __DBG_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define prnt(_x_, ...)    do {                                                          \
                                fprintf(stderr,"["#_x_" @%3d] : ", __LINE__);           \
                                fprintf(stderr, __VA_ARGS__);                           \
                                fprintf(stderr, "\n");                                  \
                          } while(0)

#define err(...)    prnt(ERR, __VA_ARGS__)

#define info(...)   prnt(INF, __VA_ARGS__)

#define perr(...)       do {                                                            \
                                int err = errno;                                        \
                                fprintf(stderr,"[ERRP @%3d] : ", __LINE__);             \
                                fprintf(stderr, __VA_ARGS__);                           \
                                fprintf(stderr, " : %s", strerror(err));                \
                                fprintf(stderr, "\n");                                  \
                        } while(0)

#define CHK_FOR_NULL_N_RET_VAL(_val_, _retval_, ...)    if((_val_) == NULL) {           \
                                                            err(__VA_ARGS__);           \
                                                            return (_retval_);          \
                                                        }

#define CHK_FOR_NULL_N_RET(_val_, ...) CHK_FOR_NULL_N_RET_VAL(_val_, -1, __VA_ARGS__)
#define CHK_FOR_NULL_N_RET_ERR(_val_) CHK_FOR_NULL_N_RET_VAL(_val_, "Error NULL == " #_val_)

#define CHK_FOR_NULL_ARG_N_RET_VAL(_val_, _ret_) CHK_FOR_NULL_N_RET_VAL(_val_, _ret_, "Invalid Arg " #_val_ )
#define CHK_FOR_NULL_ARG_N_RET_ERR(_val_) CHK_FOR_NULL_ARG_N_RET_VAL(_val_, (-1))


#ifdef DEBUG
#define dbg(...)    prnt(DBG, __VA_ARGS__)
#else
#define dbg(...)
#endif

#endif /* __DBG_H__ */