/* Copyright (C) 2006 MySQL AB & MySQL Finland AB & TCX DataKonsult AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* Read record based on a key */

#include "maria_def.h"
#include "ma_rt_index.h"

	/* Read a record using key */
	/* Ordinary search_flag is 0 ; Give error if no record with key */

int maria_rkey(MARIA_HA *info, uchar *buf, int inx, const uchar *key,
               key_part_map keypart_map, enum ha_rkey_function search_flag)
{
  uchar *key_buff;
  MARIA_SHARE *share= info->s;
  MARIA_KEYDEF *keyinfo;
  HA_KEYSEG *last_used_keyseg;
  uint pack_key_length, use_key_length, nextflag;
  DBUG_ENTER("maria_rkey");
  DBUG_PRINT("enter", ("base: 0x%lx  buf: 0x%lx  inx: %d  search_flag: %d",
                       (long) info, (long) buf, inx, search_flag));

  if ((inx = _ma_check_index(info,inx)) < 0)
    DBUG_RETURN(my_errno);

  info->update&= (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);
  info->last_key_func= search_flag;
  keyinfo= share->keyinfo + inx;

  if (info->once_flags & USE_PACKED_KEYS)
  {
    info->once_flags&= ~USE_PACKED_KEYS;	/* Reset flag */
    /*
      key is already packed!;  This happens when we are using a MERGE TABLE
      In this key 'key_part_map' is the length of the key !
    */
    key_buff= info->lastkey+info->s->base.max_key_length;
    pack_key_length= keypart_map;
    bmove(key_buff, key, pack_key_length);
    last_used_keyseg= info->s->keyinfo[inx].seg + info->last_used_keyseg;
  }
  else
  {
    DBUG_ASSERT(keypart_map);
    /* Save the packed key for later use in the second buffer of lastkey. */
    key_buff=info->lastkey+info->s->base.max_key_length;
    pack_key_length= _ma_pack_key(info,(uint) inx, key_buff, key,
                                  keypart_map, &last_used_keyseg);
    /* Save packed_key_length for use by the MERGE engine. */
    info->pack_key_length= pack_key_length;
    info->last_used_keyseg= (uint16) (last_used_keyseg -
                                      info->s->keyinfo[inx].seg);
    DBUG_EXECUTE("key", _ma_print_key(DBUG_FILE, keyinfo->seg,
                                      key_buff, pack_key_length););
  }

  if (fast_ma_readinfo(info))
    goto err;
  if (share->concurrent_insert)
    rw_rdlock(&share->key_root_lock[inx]);

  nextflag=maria_read_vec[search_flag];
  use_key_length=pack_key_length;
  if (!(nextflag & (SEARCH_FIND | SEARCH_NO_FIND | SEARCH_LAST)))
    use_key_length=USE_WHOLE_KEY;

  switch (info->s->keyinfo[inx].key_alg) {
#ifdef HAVE_RTREE_KEYS
  case HA_KEY_ALG_RTREE:
    if (maria_rtree_find_first(info,inx,key_buff,use_key_length,nextflag) < 0)
    {
      maria_print_error(info->s, HA_ERR_CRASHED);
      my_errno= HA_ERR_CRASHED;
      info->cur_row.lastpos= HA_OFFSET_ERROR;
    }
    break;
#endif
  case HA_KEY_ALG_BTREE:
  default:
    if (!_ma_search(info, keyinfo, key_buff, use_key_length,
                    maria_read_vec[search_flag],
                    info->s->state.key_root[inx]) &&
        share->concurrent_insert)
    {
      /*
        Found a key, but it might not be usable. We cannot use rows that
        are inserted by other threads after we got our table lock
        ("concurrent inserts"). The record may not even be present yet.
        Keys are inserted into the index(es) before the record is
        inserted into the data file. When we got our table lock, we
        saved the current data_file_length. Concurrent inserts always go
        to the end of the file. So we can test if the found key
        references a new record.
      */
      if (info->cur_row.lastpos >= info->state->data_file_length)
      {
        /* The key references a concurrently inserted record. */
        if (search_flag == HA_READ_KEY_EXACT &&
            last_used_keyseg == keyinfo->seg + keyinfo->keysegs)
        {
          /* Simply ignore the key if it matches exactly. (Bug #29838) */
          my_errno= HA_ERR_KEY_NOT_FOUND;
          info->cur_row.lastpos= HA_OFFSET_ERROR;
        }
        else
        {
          /*
            If searching for a partial key (or using >, >=, < or <=) and
            the data is outside of the data file, we need to continue
            searching for the first key inside the data file.
          */
          do
          {
            uint not_used[2];
            /*
              Skip rows that are inserted by other threads since we got
              a lock. Note that this can only happen if we are not
              searching after a full length exact key, because the keys
              are sorted according to position.
            */
            if  (_ma_search_next(info, keyinfo, info->lastkey,
                                 info->lastkey_length,
                                 maria_readnext_vec[search_flag],
                                 info->s->state.key_root[inx]))
              break;                          /* purecov: inspected */
            /*
              Check that the found key does still match the search.
              _ma_search_next() delivers the next key regardless of its
              value.
            */
            if (search_flag == HA_READ_KEY_EXACT &&
                ha_key_cmp(keyinfo->seg, key_buff, info->lastkey,
                           use_key_length, SEARCH_FIND, not_used))
            {
              /* purecov: begin inspected */
              my_errno= HA_ERR_KEY_NOT_FOUND;
              info->cur_row.lastpos= HA_OFFSET_ERROR;
              break;
              /* purecov: end */
            }
          } while (info->cur_row.lastpos >= info->state->data_file_length);
        }
      }
    }
  }
  if (share->concurrent_insert)
    rw_unlock(&share->key_root_lock[inx]);

  if (info->cur_row.lastpos == HA_OFFSET_ERROR)
  {
    fast_ma_writeinfo(info);
    goto err;
  }

  /* Calculate length of the found key;  Used by maria_rnext_same */
  if ((keyinfo->flag & HA_VAR_LENGTH_KEY) && last_used_keyseg)
    info->last_rkey_length= _ma_keylength_part(keyinfo, info->lastkey,
					       last_used_keyseg);
  else
    info->last_rkey_length= pack_key_length;

  /* Check if we don't want to have record back, only error message */
  if (!buf)
  {
    fast_ma_writeinfo(info);
    DBUG_RETURN(0);
  }
  if (!(*info->read_record)(info, buf, info->cur_row.lastpos))
  {
    info->update|= HA_STATE_AKTIV;		/* Record is read */
    DBUG_RETURN(0);
  }

  info->cur_row.lastpos= HA_OFFSET_ERROR;      /* Didn't find row */

err:
  /* Store last used key as a base for read next */
  memcpy(info->lastkey,key_buff,pack_key_length);
  info->last_rkey_length= pack_key_length;
  bzero((char*) info->lastkey+pack_key_length,info->s->base.rec_reflength);
  info->lastkey_length=pack_key_length+info->s->base.rec_reflength;

  if (search_flag == HA_READ_AFTER_KEY)
    info->update|=HA_STATE_NEXT_FOUND;		/* Previous gives last row */
  DBUG_RETURN(my_errno);
} /* _ma_rkey */
