/*
 * alig.h
 *
 *  Created on: Dec 12, 2013
 *      Author: rmoreno
 */

#ifndef ALIG_H_
#define ALIG_H_

#include <assert.h>

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>

#include <omp.h>

#include <bioformats/bam/samtools/bam.h>
#include <bioformats/bam/bam_file.h>
#include <aligners/bwt/genome.h>
#include "aux/aux_common.h"
#include "aux/aux_library.h"
#include "aux/timestats.h"
#include "alig_region.h"

#define ALIG_LIST_COUNT_THRESHOLD_TO_WRITE 1000

#define ALIG_REFERENCE_ADDITIONAL_OFFSET 100

/**
 * Time measures
 */
#define D_TIME_DEBUG
#ifdef D_TIME_DEBUG
	//BAM I/0
	#define D_SLOT_READ 			0
	#define D_SLOT_WRITE 			1

	//GENERAL
	#define D_SLOT_TOTAL			2			//Total time elapsed
	#define D_SLOT_PROCCESS			3			//General processing for every read (realigned or not)
	#define D_SLOT_INIT				4			//Initialization aligner time


	//REALIGN
	#define D_SLOT_HAPLO_GET 		5			//Obtaining haplos from one read
	#define D_SLOT_REALIG_PER_HAPLO 6			//Read alig time / number haplotypes

#endif


/**
 * REALIGNMENT CONTEXT
 */

typedef struct {
	//BAM files
	bam_file_t *in_bam_f;
	bam_file_t *out_bam_f;

	//Reference genome
	genome_t *genome;

	//BAM lists
	array_list_t *process_list;

	//Alignments readed
	size_t read_count;

	//Auxiliar read ptr
	bam1_t *last_read;
	size_t last_read_bytes;

	//Haplotypes
	array_list_t *haplo_list;

	//Current region
	alig_region_t region;


} alig_context_t;


/**
 * INTERVAL STATUS
 */

typedef enum {
	NO_INTERVAL,
	INTERVAL
} alig_status;

/**
 * CONTEXT
 */

EXTERNC ERROR_CODE alig_init(alig_context_t *context, bam_file_t *in_bam_f, bam_file_t *out_bam_f, genome_t *genome);
EXTERNC ERROR_CODE alig_destroy(alig_context_t *context);
EXTERNC ERROR_CODE alig_validate(alig_context_t *context);

/**
 * REGION OPERATIONS
 */

EXTERNC ERROR_CODE alig_region_next(alig_context_t *context);
EXTERNC ERROR_CODE alig_region_indel_realignment(alig_context_t *context);
EXTERNC ERROR_CODE alig_region_write(alig_context_t *context);

EXTERNC ERROR_CODE alig_bam_file2(char *bam_path, char *ref_name, char *ref_path);

/**
 * HAPLO OPERATIONS
 */

EXTERNC ERROR_CODE alig_haplo_process(alig_context_t *context);

/**
 * BAM REALIGN
 */

EXTERNC ERROR_CODE alig_bam_file(char *bam_path, char *ref_name, char *ref_path);
EXTERNC ERROR_CODE alig_bam_list(array_list_t *bam_list, genome_t* ref);
EXTERNC ERROR_CODE alig_bam_list_to_disk(array_list_t *bam_list, bam_file_t *bam_f);

EXTERNC ERROR_CODE alig_bam_list_realign(array_list_t *bam_list, array_list_t *haplotype_list, genome_t* ref);

EXTERNC ERROR_CODE alig_bam_list_get_next(bam_file_t *bam_f, array_list_t *out_list);


#endif /* ALIG_H_ */
