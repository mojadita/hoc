/* test-hash.cc -- tests para el modulo hash.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Jun  2 09:27:14 -05 2025
 * copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <iostream>

#include <gtest/gtest.h>
//#include <gmock/gmock.h>

#include "crc.h"
#include "crc32ieee802_3.c"

#include "hashP.h"

extern "C" {

    int test_hash(const char *s)
    {
        CRC_STATE st = 0xffffffff;
        st = do_crc(st, s, strlen(s), crc32ieee802_3);
        return st;
    }

} /* extern "C" */

#define BUCKETS 31

/* se incluyen tests de unidad para el modulo hash.c */

/* para una referencia de Google Test ver https://google.github.io/googletest/ */

struct TestHashAllocation: testing::Test {
    struct hash_map *uut;

    TestHashAllocation() {
        uut = new_hash_map(BUCKETS, NULL, NULL);
    }
};

/*  Primer test para la funcion new_hash_map()  */
/*  Clase: TestHash    Metodo: testNewHashMapAllocationOK  */
/*  Los primeros test son para comprobar la correcta inicializacion  */
TEST_F(TestHashAllocation, testNewHashMapAllocationOK)
{
    ASSERT_NE(uut, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapAllocationBucketsOK)
{
    ASSERT_NE(uut->buckets, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapSizeOK)
{
    ASSERT_EQ(uut->size, 0);
}

TEST_F(TestHashAllocation, testNewHashMapBucketsLenOK)
{
    ASSERT_EQ(uut->buckets_len, BUCKETS);
}

TEST_F(TestHashAllocation, testNewHashMapElem0Ok)
{
    ASSERT_EQ(uut->buckets[0].elem_len, 0);
    ASSERT_EQ(uut->buckets[0].elem[0].key, NULL);
    ASSERT_EQ(uut->buckets[0].elem[0].val, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapElemNOk)
{
    ASSERT_EQ(uut->buckets[BUCKETS - 1].elem_len, 0);
    ASSERT_EQ(uut->buckets[BUCKETS - 1].elem[0].key, NULL);
    ASSERT_EQ(uut->buckets[BUCKETS - 1].elem[0].val, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapHashNULL)
{
    ASSERT_EQ(uut->hash, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapCompNULL)
{
    ASSERT_EQ(uut->equal, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapHashNotNULL)
{
    delete uut;
    uut = new_hash_map(BUCKETS, test_hash, NULL);
    ASSERT_EQ(uut->hash, test_hash);
    ASSERT_EQ(uut->equal, NULL);
}

TEST_F(TestHashAllocation, testNewHashMapCompNotNULL)
{
    delete uut;
    uut = new_hash_map(BUCKETS, NULL, strcmp);
    ASSERT_EQ(uut->hash, NULL);
    ASSERT_EQ(uut->equal, (equal_f)strcmp);
}

/*******************************************************************************/
/*     Tests para la funcion del_hash_map                                      */
/*******************************************************************************/

struct TestHashFunctionality: testing::Test {
    struct hash_map *uut;

    TestHashFunctionality() {
        uut = new_hash_map(BUCKETS, (hash_f)test_hash, (equal_f)strcmp);
    }
};

void f_to_apply(struct hash_map *m, struct pair *p, void *c)
{
    int count = ++*(int *)c;
    std::cout
        << "<"         << p->key
        << ", "        << (char *) p->val
        << "> count: " << count
        << std::endl;
} /* f_to_apply */

TEST_F(TestHashFunctionality, testApply)
{
    ASSERT_NE(hash_map_put(uut, "pepito",    (void *)"a"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "juanito",   (void *)"b"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "alfonsito", (void *)"c"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "pedrito",   (void *)"d"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "adolfito",    (void *)"e"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "carlitos",   (void *)"f"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "bonifacito", (void *)"g"), (struct pair *)NULL);
    ASSERT_NE(hash_map_put(uut, "danielito",   (void *)"h"), (struct pair *)NULL);

    int count = 0;

    hash_map_apply(uut, f_to_apply, (void *) &count);

    ASSERT_EQ(count, hash_map_size(uut));
} /* TEST_F(TestHashWorking, testFunctionApply) */
