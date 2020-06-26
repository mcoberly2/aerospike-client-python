# -*- coding: utf-8 -*-
import pytest
import sys
import random
import time
from .test_base_class import TestBaseClass
from aerospike import exception as e
from aerospike_helpers.operations import hll_operations

import pytest

aerospike = pytest.importorskip("aerospike")
try:
    import aerospike
except:
    print("Please install aerospike python client.")
    sys.exit(1)


class TestHLL(object):

    @pytest.fixture(autouse=True)
    def setup(self, request, as_connection):
        self.test_keys = []
        # rec = {'list_bin': [1, 2, 3]}
        # as_connection.put(key, rec)

        for x in range(5):

            key = ('test', 'demo', x)
            record = {'label': x}

            as_connection.put(key, record)

            # print(['key%s' % str(i) for i in range(100)])

            ops = [
                hll_operations.hll_init('hll_bine', 10),
                hll_operations.hll_add('hll_bin', ['key%s' % str(i) for i in range(x + 1)], 8),
                hll_operations.hll_add_mh('mh_bin', ['key%s' % str(i) for i in range(x + 1)], 6, 12),
                hll_operations.hll_add('hll_binl', ['key%s' % str(i) for i in range(100)], 8),
                hll_operations.hll_add('hll_binu', ['key6', 'key7', 'key8', 'key9', 'key10'], 10),
            ]

            self.test_keys.append(key)
            _, _, _ = as_connection.operate(key, ops)

        def teardown():
            """
            Teardown method.
            """
            for key in self.test_keys:
                try:
                    as_connection.remove(key)
                except e.RecordNotFound:
                    pass

        request.addfinalizer(teardown)

    @pytest.mark.parametrize("policy, expected_result", [
        (None, 3),
        ({'flags': aerospike.HLL_WRITE_CREATE_ONLY}, 3),
        ({'flags': aerospike.HLL_WRITE_NO_FAIL}, 3)
    ])
    def test_pos_hll_add(self, policy, expected_result):
        """
        Invoke hll_add() creating a new HLL.
        """
        ops = [
            hll_operations.hll_add('new_bin', ['key1', 'key2', 'key3'], 8, policy)
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)

        assert res['new_bin'] == expected_result

    @pytest.mark.parametrize("policy, expected_result", [
        ({'flags': aerospike.HLL_WRITE_UPDATE_ONLY}, e.InvalidRequest)
    ])
    def test_neg_hll_add(self, policy, expected_result):
        """
        Invoke hll_add() creating a new HLL with expected failures.
        """
        ops = [
            hll_operations.hll_add('new_bin', ['key1', 'key2', 'key3'], 8, policy)
        ]

        with pytest.raises(expected_result):
            self.as_connection.operate(self.test_keys[0], ops)

    def test_pos_hll_add_mh(self):
        """
        Invoke hll_add_mh() creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_add_mh('new_bin', ['key1', 'key2', 'key3'], 6, 8)
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)

        assert res['new_bin'] == 3
        #TODO use get_count to actually check the new hll
        #(key, _, bins) = self.as_connection.get(self.test_key)

    @pytest.mark.parametrize("policy, expected_result", [
        ({'flags': aerospike.HLL_WRITE_CREATE_ONLY}, e.BinExistsError)
    ])
    def test_neg_hll_add_mh(self, policy, expected_result):
        """
        Invoke hll_add_mh() failing to creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_add_mh('hll_binl', ['key101', 'key102', 'key103'], 6, 8, policy)
        ]

        with pytest.raises(expected_result):
            self.as_connection.operate(self.test_keys[0], ops)

    def test_pos_hll_get_count(self):
        """
        Invoke hll_get_count() to check an HLL's count.
        """
        ops = [
            hll_operations.hll_get_count('hll_bin')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[2], ops)
        assert res['hll_bin'] == 3

    def test_neg_hll_get_count(self):
        """
        Invoke hll_get_count() with a non existent bin.
        """
        ops = [
            hll_operations.hll_get_count('fake_hll_bin')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[2], ops)
        assert res['fake_hll_bin'] == None

    def test_pos_hll_describe(self):
        """
        Invoke hll_describe() and check index and min hash bits.
        """
        ops = [
            hll_operations.hll_describe('mh_bin')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)
        assert res['mh_bin'] == [6, 12]

    def test_neg_hll_describe(self):
        """
        Invoke hll_describe() with a non existent hll bin.
        """
        ops = [
            hll_operations.hll_describe('fake_hll_bin')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)
        assert res['fake_hll_bin'] == None

    def test_pos_hll_fold(self):
        """
        Invoke hll_fold().
        """
        ops = [
            hll_operations.hll_fold('hll_bin', 6),
            hll_operations.hll_describe('hll_bin')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[2], ops)
        assert res['hll_bin'] == [6, 0]

    @pytest.mark.parametrize("index_bits, bin, expected_result", [
        (2, 'hll_bin', e.InvalidRequest),
        (4, 'mh_bin', e.OpNotApplicable)
    ])
    def test_neg_hll_fold(self, index_bits, bin, expected_result):
        """
        Invoke hll_fold() expecting errors.
        """
        ops = [
            hll_operations.hll_fold(bin, index_bits),
            hll_operations.hll_describe(bin)
        ]

        with pytest.raises(expected_result):
            self.as_connection.operate(self.test_keys[2], ops)

    def test_pos_hll_get_intersect_count(self):
        """
        Invoke hll_get_intersect_count().
        """

        records =  [record[2]['hll_binl'] for record in self.as_connection.get_many(self.test_keys[:1])]

        ops = [
            hll_operations.hll_get_intersect_count('hll_binl', records)
        ]

        _, _, res = self.as_connection.operate(self.test_keys[4], ops)
        assert res['hll_binl'] >= 97 or res['hll_binl'] <= 103

    def test_pos_hll_get_intersect_count_mh(self):
        """
        Invoke hll_get_intersect_count() on min hash bins.
        """
        records =  [record[2]['mh_bin'] for record in self.as_connection.get_many(self.test_keys[:4])]

        ops = [
            hll_operations.hll_get_intersect_count('mh_bin', records)
        ]

        _, _, res = self.as_connection.operate(self.test_keys[4], ops)
        assert res['mh_bin'] == 1

    @pytest.mark.parametrize("hll_bins, bin, expected_result", [
        (5, 'hll_binl', e.InvalidRequest)
    ])
    def test_neg_hll_get_intersect_count(self, hll_bins, bin, expected_result):
        """
        Invoke hll_get_intersect_count().
        """

        records =  [record[2][bin] for record in self.as_connection.get_many(self.test_keys[:hll_bins - 1])]

        ops = [
            hll_operations.hll_get_intersect_count(bin, records)
        ]

        with pytest.raises(expected_result):
            self.as_connection.operate(self.test_keys[4], ops)

    def test_pos_hll_get_similarity(self):
        """
        Invoke hll_get_similarity().
        """

        records =  [record[2]['hll_binl'] for record in self.as_connection.get_many(self.test_keys)]

        ops = [
            hll_operations.hll_get_similarity('hll_bin', [records[1]])
        ]

        _, _, res = self.as_connection.operate(self.test_keys[4], ops)
        assert res['hll_bin'] >= 0.050 and res['hll_bin'] <= 0.059

    @pytest.mark.parametrize("hll_bins, bin, expected_result", [
        (5, 'hll_binl', e.InvalidRequest)
    ])
    def test_neg_hll_get_similarity(self, hll_bins, bin, expected_result):
        """
        Invoke hll_get_similarity() with errors.
        """

        records =  [record[2][bin] for record in self.as_connection.get_many(self.test_keys[:hll_bins - 1])]

        ops = [
            hll_operations.hll_get_similarity(bin, records)
        ]

        with pytest.raises(expected_result):
            self.as_connection.operate(self.test_keys[4], ops)

    def test_pos_hll_get_union(self):
        """
        Invoke hll_get_union().
        """

        records =  [record[2]['hll_binu'] for record in self.as_connection.get_many(self.test_keys)]

        ops = [
            hll_operations.hll_get_union('hll_bin', [records[4]])
        ]

        _, _, union_hll = self.as_connection.operate(self.test_keys[4], ops)

        ops = [
            hll_operations.hll_get_intersect_count('hll_bin', [union_hll['hll_bin']]),
            hll_operations.hll_get_intersect_count('hll_binu', [union_hll['hll_bin']])
        ]

        _, _, res = self.as_connection.operate(self.test_keys[4], ops)

        assert res['hll_bin'] == 5
        assert res['hll_binu'] == 5

    def test_pos_hll_get_union_count(self):
        """
        Invoke hll_get_union_count().
        """

        records =  [record[2]['hll_bin'] for record in self.as_connection.get_many(self.test_keys)]

        ops = [
            hll_operations.hll_get_union_count('hll_binu', records)
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)

        assert res['hll_binu'] == 10

    def test_neg_hll_get_union_count(self):
        """
        Invoke hll_get_union_count() with errors.
        """

        records =  []

        ops = [
            hll_operations.hll_get_union_count('hll_binu', records)
        ]

        with pytest.raises(e.InvalidRequest):
            self.as_connection.operate(self.test_keys[0], ops)

    def test_pos_hll_init_mh(self):
        """
        Invoke hll_add_mh() creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_init_mh('new_mhbin', 6, 8)
        ]

        _, _, _ = self.as_connection.operate(self.test_keys[0], ops)

        record = self.as_connection.get(self.test_keys[0])
        assert record[2]['new_mhbin']

    @pytest.mark.parametrize("hll_bins, bin, expected_result", [
        (5, 'hll_binl', e.InvalidRequest)
    ])
    def test_neg_hll_init_mh(self, hll_bins, bin, expected_result):
        """
        Invoke hll_add_mh() creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_init_mh('new_mhbin', 6, 8)
        ]

        _, _, _ = self.as_connection.operate(self.test_keys[0], ops)

        record = self.as_connection.get(self.test_keys[0])
        assert record[2]['new_mhbin']

    def test_pos_hll_init_mh(self):
        """
        Invoke hll_add_mh() creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_init_mh('new_mhbin', 6, 8)
        ]

        _, _, _ = self.as_connection.operate(self.test_keys[0], ops)

        record = self.as_connection.get(self.test_keys[0])
        assert record[2]['new_mhbin']

    def test_pos_hll_init(self):
        """
        Invoke hll_add_mh() creating a new min hash HLL.
        """
        ops = [
            hll_operations.hll_init('new_hll', 6)
        ]

        _, _, _ = self.as_connection.operate(self.test_keys[0], ops)

        record = self.as_connection.get(self.test_keys[0])
        assert record[2]['new_hll']

    def test_pos_hll_refresh_count(self):
        """
        Invoke hll_refresh_count().
        """
        ops = [
            hll_operations.hll_refresh_count('hll_binl')
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)
        assert res['hll_binl'] == 97

    def test_pos_hll_set_union(self):
        """
        Invoke hll_set_union().
        """

        records =  [record[2]['hll_binu'] for record in self.as_connection.get_many(self.test_keys)]

        ops = [
            hll_operations.hll_set_union('hll_bin', [records[4]])
        ]

        _, _, _ = self.as_connection.operate(self.test_keys[4], ops)
        
        union_hll = self.as_connection.get(self.test_keys[4])[2]['hll_bin']

        ops = [
            hll_operations.hll_get_intersect_count('hll_bin', [union_hll]),
            hll_operations.hll_get_intersect_count('hll_binu', [union_hll])
        ]

        _, _, res = self.as_connection.operate(self.test_keys[4], ops)

        assert res['hll_bin'] == 10
        assert res['hll_binu'] == 5

    def test_pos_hll_update(self):
        """
        Invoke hll_update() creating a new HLL.
        """
        ops = [
            hll_operations.hll_update('hll_bine', ['key1', 'key2', 'key3'])
        ]

        _, _, res = self.as_connection.operate(self.test_keys[0], ops)

        assert res['hll_bine'] == 3