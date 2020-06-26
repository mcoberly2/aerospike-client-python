/*******************************************************************************
* Copyright 2013-2020 Aerospike, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
******************************************************************************/


#include <Python.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <aerospike/as_operations.h>
#include <aerospike/as_hll_operations.h>
#include <aerospike/as_cdt_ctx.h>

#include "client.h"
#include "conversions.h"
#include "exceptions.h"
#include "policy.h"
#include "serializer.h"
#include "cdt_hll_operations.h"
#include "cdt_operation_utils.h"

#define AS_PY_HLL_POLICY "hll_policy"
#define AS_PY_HLL_INDEX_BIT_COUNT "index_bit_count"
#define AS_PY_HLL_MH_BIT_COUNT_KEY "mh_bit_count"


static as_status
get_hll_policy(as_error* err, PyObject* op_dict, as_hll_policy* policy, bool* found);

static as_status
add_op_hll_add(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_init(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_get_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_add_mh(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_describe(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_fold(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_get_intersect_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_get_similarity(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_get_union(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_get_union_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_init_mh(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_refresh_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_set_union(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

static as_status
add_op_hll_update(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type);

as_status
add_new_hll_op(AerospikeClient* self, as_error* err, PyObject* op_dict, as_vector* unicodeStrVector,
	    as_static_pool* static_pool, as_operations* ops, long operation_code, long* ret_type, int serializer_type)

{
    char* bin = NULL;

    if (get_bin(err, op_dict, unicodeStrVector, &bin) != AEROSPIKE_OK) {
        return err->code;
    }

    switch(operation_code) {
    	case OP_HLL_ADD:
    		return add_op_hll_add(self, err, bin, op_dict, ops, static_pool, serializer_type);
        
        case OP_HLL_INIT:
            return add_op_hll_init(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_GET_COUNT:
            return add_op_hll_get_count(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_ADD_MH:
            return add_op_hll_add_mh(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_DESCRIBE:
            return add_op_hll_describe(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_FOLD:
            return add_op_hll_fold(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_GET_INTERSECT_COUNT:
            return add_op_hll_get_intersect_count(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_GET_SIMILARITY:
            return add_op_hll_get_similarity(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_GET_UNION:
            return add_op_hll_get_union(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_GET_UNION_COUNT:
            return add_op_hll_get_union_count(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_INIT_MH:
            return add_op_hll_init_mh(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_REFRESH_COUNT:
            return add_op_hll_refresh_count(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_SET_UNION:
            return add_op_hll_set_union(self, err, bin, op_dict, ops, static_pool, serializer_type);

        case OP_HLL_UPDATE:
            return add_op_hll_update(self, err, bin, op_dict, ops, static_pool, serializer_type);

        default:
            // This should never be possible since we only get here if we know that the operation is valid.
            return as_error_update(err, AEROSPIKE_ERR_PARAM, "Unknown operation");
    }

	return err->code;
}

static as_status
add_op_hll_add(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_hll_policy hll_policy;
    int index_bit_count;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    bool policy_in_use = false;

    if (get_int(err, AS_PY_HLL_INDEX_BIT_COUNT, op_dict, &index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_add(ops, bin, NULL, policy_in_use ? &hll_policy : NULL, value_list, index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_add_mh(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_hll_policy hll_policy;
    int index_bit_count;
    int mh_bit_count;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    bool policy_in_use = false;

    if (get_int(err, AS_PY_HLL_INDEX_BIT_COUNT, op_dict, &index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_int(err, AS_PY_HLL_MH_BIT_COUNT_KEY, op_dict, &mh_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_add_mh(ops, bin, NULL, policy_in_use ? &hll_policy : NULL, value_list, index_bit_count, mh_bit_count) != AEROSPIKE_OK) {
        //TODO destroy val_list
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_init(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_hll_policy hll_policy;
    int index_bit_count;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    bool policy_in_use = false;

    if (get_int(err, AS_PY_HLL_INDEX_BIT_COUNT, op_dict, &index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_init(ops, bin, NULL, NULL, index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_init_mh(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_hll_policy hll_policy;
    int index_bit_count;
    int mh_bit_count;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    bool policy_in_use = false;

    if (get_int(err, AS_PY_HLL_INDEX_BIT_COUNT, op_dict, &index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_int(err, AS_PY_HLL_MH_BIT_COUNT_KEY, op_dict, &mh_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_init_mh(ops, bin, NULL, NULL, index_bit_count, mh_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_get_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_get_count(ops, bin, NULL) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_describe(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_describe(ops, bin, NULL) != AEROSPIKE_OK){
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_fold(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    int index_bit_count;

    if (get_int(err, AS_PY_HLL_INDEX_BIT_COUNT, op_dict, &index_bit_count) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_fold(ops, bin, NULL, index_bit_count) != AEROSPIKE_OK){
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_get_intersect_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_get_intersect_count(ops, bin, NULL, value_list) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    if (value_list) {
        as_val_destroy(value_list);
    }

    return err->code;
}

static as_status
add_op_hll_get_similarity(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_get_similarity(ops, bin, NULL, value_list) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    if (value_list) {
        as_val_destroy(value_list);
    }

    return err->code;
}

static as_status
add_op_hll_get_union(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_get_union(ops, bin, NULL, value_list) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    if (value_list) {
        as_val_destroy(value_list);
    }

    return err->code;
}

static as_status
add_op_hll_get_union_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_get_union_count(ops, bin, NULL, value_list) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    if (value_list) {
        as_val_destroy(value_list);
    }

    return err->code;
}

static as_status
add_op_hll_refresh_count(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_cdt_ctx ctx;
    bool ctx_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_refresh_count(ops, bin, NULL) != AEROSPIKE_OK){
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
add_op_hll_set_union(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    as_hll_policy hll_policy;
    bool policy_in_use = false;

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        goto cleanup;
    }

    if (as_operations_hll_set_union(ops, bin, NULL, NULL, value_list) != AEROSPIKE_OK){
        goto cleanup;
    }

cleanup:
    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    if (value_list) {
        as_val_destroy(value_list);
    }

    return err->code;
}

static as_status
add_op_hll_update(AerospikeClient* self, as_error* err, char* bin,
        PyObject* op_dict, as_operations* ops,
        as_static_pool* static_pool, int serializer_type)
{
    as_list* value_list = NULL;
    as_hll_policy hll_policy;
    as_cdt_ctx ctx;
    bool ctx_in_use = false;
    bool policy_in_use = false;

    if (get_hll_policy(err, op_dict, &hll_policy, &policy_in_use) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_cdt_ctx(self, err, &ctx, op_dict, &ctx_in_use, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (get_val_list(self, err, AS_PY_VALUES_KEY, op_dict, &value_list, static_pool, serializer_type) != AEROSPIKE_OK) {
        return err->code;
    }

    if (as_operations_hll_update(ops, bin, NULL, &hll_policy, value_list) != AEROSPIKE_OK) {
        return err->code;
    }

    if (ctx_in_use) {
        as_cdt_ctx_destroy(&ctx);
    }

    return err->code;
}

static as_status
get_hll_policy(as_error* err, PyObject* op_dict, as_hll_policy* policy, bool* found) {
    *found = false;

    PyObject* hll_policy = PyDict_GetItemString(op_dict, AS_PY_HLL_POLICY);

    if (hll_policy) {
        if (pyobject_to_hll_policy(err, hll_policy, policy) != AEROSPIKE_OK) {
            return err -> code;
        }
        *found = true;
    }

    return AEROSPIKE_OK;
}