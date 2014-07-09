#ifndef _SUFFIX_MNG_H
#define _SUFFIX_MNG_H

#include "dna/clasp_v1_1/info.h"
#include "dna/clasp_v1_1/debug.h"
#include "dna/clasp_v1_1/container.h"
#include "dna/clasp_v1_1/fileio.h"
#include "dna/clasp_v1_1/sltypes.h"
#include "dna/clasp_v1_1/slchain.h"
#include "dna/clasp_v1_1/rangetree.h"
#include "dna/clasp_v1_1/basic-types.h"
#include "dna/clasp_v1_1/clasp.h"

#include "dna/sa_dna_commons.h"
#include "dna/doscadfun.h"

//--------------------------------------------------------------------
// suffix_mng_t struct
//--------------------------------------------------------------------

typedef struct suffix_mng {
  int num_seeds;
  int num_chroms;
  Container *subject;
  linked_list_t **suffix_lists;
} suffix_mng_t;

suffix_mng_t *suffix_mng_new(sa_genome3_t *genome);
void suffix_mng_free(suffix_mng_t *p);
void suffix_mng_clear(suffix_mng_t *p);

void suffix_mng_update(int chrom, size_t read_start, size_t read_end, 
		       size_t genome_start, size_t genome_end, 
		       suffix_mng_t *p);

void suffix_mng_create_cals(fastq_read_t *read, int min_area, int strand, 
			    sa_index3_t *sa_index, array_list_t *cal_list,
			    suffix_mng_t *p);

void suffix_mng_search_seeds(fastq_read_t *read, int min_area, int strand, 
			     sa_index3_t *sa_index, array_list_t *cal_list,
			     suffix_mng_t *p);

void suffix_mng_display(suffix_mng_t *p);

//--------------------------------------------------------------------
//--------------------------------------------------------------------

#endif // _SUFFIX_MNG_H
