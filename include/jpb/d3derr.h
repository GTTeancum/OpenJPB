#ifndef JPB_D3DERR_H
#define JPB_D3DERR_H

struct _d3derr {
    unsigned err;
    char *errmsg;
};

static_assert(sizeof(_d3derr) == 16, "_d3derr PDB layout changed");

extern _d3derr alldderrs[199];

void d3derr(unsigned hr, char *msg, unsigned line, char *file);
char *dderrmsg(unsigned hr);

#endif
