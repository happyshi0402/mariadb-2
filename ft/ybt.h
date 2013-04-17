/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ifndef TOKU_YBT_H
#define TOKU_YBT_H

#ident "$Id$"
#ident "Copyright (c) 2007-2012 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

// fttypes.h must be first to make 64-bit file mode work right in linux.
#include "fttypes.h"
#include <db.h>


DBT *toku_init_dbt(DBT *);
DBT *toku_init_dbt_flags(DBT *, uint32_t flags);
void toku_destroy_dbt(DBT *);
DBT *toku_fill_dbt(DBT *dbt, bytevec k, ITEMLEN len);
DBT *toku_copyref_dbt(DBT *dst, const DBT src);
DBT *toku_clone_dbt(DBT *dst, const DBT src);
int toku_dbt_set(ITEMLEN len, bytevec val, DBT *d, struct simple_dbt *sdbt);
int toku_dbt_set_value(DBT *, bytevec *val, ITEMLEN vallen, void **staticptrp, bool ybt1_disposable);
void toku_sdbt_cleanup(struct simple_dbt *sdbt);


#endif
