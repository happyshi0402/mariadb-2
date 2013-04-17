/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007-2012 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."
// verify the range tree memory size function

#include "test.h"

static void my_init_range(toku_range *range, int *left, int *right, int data) {
    range->ends.left = (toku_point *) left;
    range->ends.right = (toku_point *) right;
    range->data = data;
}

int main(int argc, const char *argv[]) {
    int r;

    parse_args(argc, argv);

    toku_range_tree *tree;
    r = toku_rt_create(&tree, int_cmp, char_cmp, false, test_incr_memory_size, test_decr_memory_size, NULL); 
    CKERR(r);

    size_t last_memory_size = toku_rt_memory_size(tree); 
    
    uint32_t last_size;

    const int nranges = 10;
    int nums[nranges];
    for (int i = 0; i < nranges; i++) {
        last_size = toku_rt_get_size(tree);
        assert(last_size == (uint32_t) i);

        nums[i] = i;
        toku_range range; my_init_range(&range, &nums[i], &nums[i], 'a');
        r = toku_rt_insert(tree, &range); CKERR(r);

        size_t memory_size = toku_rt_memory_size(tree);
        assert(memory_size >= last_memory_size);
        if (verbose)
            printf("%lu\n", memory_size);
        last_memory_size = memory_size;
    }

    toku_rt_clear(tree);
    
    last_size = toku_rt_get_size(tree);
    assert(last_size == 0);

    r = toku_rt_close(tree); CKERR(r);

    return 0;
}
