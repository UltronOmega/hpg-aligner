/*
 * aux_cigar.h
 *
 *  Created on: Dec 16, 2013
 *      Author: rmoreno
 */

#ifndef AUX_CIGAR_H_
#define AUX_CIGAR_H_

#include "aux_library.h"

/***************************
 * CIGAR OPERATIONS
 **************************/

typedef struct {
	uint32_t indel;
	size_t ref_pos;
} aux_indel_t;

EXTERNC ERROR_CODE cigar32_leftmost(char *ref, char *read, size_t read_l, uint32_t *cigar, size_t cigar_l, uint32_t *new_cigar, size_t *new_cigar_l);
EXTERNC ERROR_CODE cigar32_unclip(uint32_t *cigar, size_t cigar_l, uint32_t *new_cigar, size_t *new_cigar_l);
EXTERNC ERROR_CODE cigar32_reclip(uint32_t *clip_cigar, size_t clip_cigar_l, uint32_t *unclip_cigar, size_t unclip_cigar_l, uint32_t *new_cigar, size_t *new_cigar_l);
EXTERNC ERROR_CODE cigar32_count_m_blocks(uint32_t *cigar, size_t cigar_l, size_t *blocks);
EXTERNC ERROR_CODE cigar32_count_indels(uint32_t *cigar, size_t cigar_l, size_t *indels);
EXTERNC ERROR_CODE cigar32_count_all(uint32_t *cigar, size_t cigar_l, size_t *m_blocks, size_t *indels, size_t *first_indel_index);
EXTERNC ERROR_CODE cigar32_to_string(uint32_t *cigar, size_t cigar_l, char* str_cigar);
EXTERNC ERROR_CODE cigar32_create_ref(uint32_t *cigar, size_t cigar_l, char *ref, char *read, size_t length, char *new_ref, size_t *new_ref_l);
EXTERNC ERROR_CODE cigar32_shift_left_indel(uint32_t *cigar, size_t cigar_l, size_t indel_index, uint32_t *new_cigar);
EXTERNC ERROR_CODE cigar32_get_indels(size_t ref_pos, uint32_t *cigar, size_t cigar_l, aux_indel_t *out_indels);

#endif /* AUX_CIGAR_H_ */
