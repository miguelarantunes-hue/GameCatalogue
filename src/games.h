/*
 * games.h  –  Game database declarations
 */
#ifndef GAMES_H
#define GAMES_H

typedef struct {
    const char *n;   /* name  */
    const char *g;   /* genre */
    int         y;   /* year  */
} GE;

extern const GE  GDB[];
extern const int N_GDB;

#endif /* GAMES_H */