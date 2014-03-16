/* Copyright (c) 2006, 2007 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

/**
 * An enum and a struct to handle partitioning and subpartitioning.
 */
enum partition_type {
  NOT_A_PARTITION= 0,
  RANGE_PARTITION,
  HASH_PARTITION,
  LIST_PARTITION
};

enum partition_state {
  PART_NORMAL= 0,
  PART_IS_DROPPED= 1,
  PART_TO_BE_DROPPED= 2,
  PART_TO_BE_ADDED= 3,
  PART_TO_BE_REORGED= 4,
  PART_REORGED_DROPPED= 5,
  PART_CHANGED= 6,
  PART_IS_CHANGED= 7,
  PART_IS_ADDED= 8
};

/*
  This struct is used to contain the value of an element
  in the VALUES IN struct. It needs to keep knowledge of
  whether it is a signed/unsigned value and whether it is
  NULL or not.
*/

typedef struct p_elem_val
{
  longlong value;
  bool null_value;
  bool unsigned_flag;
} part_elem_value;

struct st_ddl_log_memory_entry;

class partition_element :public Sql_alloc {
public:
  List<partition_element> subpartitions;
  List<part_elem_value> list_val_list;
  ha_rows part_max_rows;
  ha_rows part_min_rows;
  longlong range_value;
  char *partition_name;
  char *tablespace_name;
  struct st_ddl_log_memory_entry *log_entry;
  char* part_comment;
  char* data_file_name;
  char* index_file_name;
  handlerton *engine_type;
  LEX_STRING connect_string;
  enum partition_state part_state;
  uint16 nodegroup_id;
  bool has_null_value;
  bool signed_flag;/* Indicate whether this partition uses signed constants */
  bool max_value;  /* Indicate whether this partition uses MAXVALUE */

  partition_element()
  : part_max_rows(0), part_min_rows(0), range_value(0),
    partition_name(NULL), tablespace_name(NULL),
    log_entry(NULL), part_comment(NULL),
    data_file_name(NULL), index_file_name(NULL),
    engine_type(NULL), part_state(PART_NORMAL),
    nodegroup_id(UNDEF_NODEGROUP), has_null_value(FALSE),
    signed_flag(FALSE), max_value(FALSE)
  {
    connect_string.str= 0;
    connect_string.length= 0;
  }
  partition_element(partition_element *part_elem)
  : part_max_rows(part_elem->part_max_rows),
    part_min_rows(part_elem->part_min_rows),
    range_value(0), partition_name(NULL),
    tablespace_name(part_elem->tablespace_name),
    part_comment(part_elem->part_comment),
    data_file_name(part_elem->data_file_name),
    index_file_name(part_elem->index_file_name),
    engine_type(part_elem->engine_type),
    connect_string(part_elem->connect_string),
    part_state(part_elem->part_state),
    nodegroup_id(part_elem->nodegroup_id),
    has_null_value(FALSE)
  {
    connect_string.str= 0;
    connect_string.length= 0;
  }
  ~partition_element() {}
};
