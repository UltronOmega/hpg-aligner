/*
 * bam_region.h
 *
 *  Created on: 24/04/2014
 *      Author: rmoreno
 */

#ifndef BAM_REGION_H_
#define BAM_REGION_H_

#include <assert.h>
#include <omp.h>

#include "aux/aux_common.h"
#include "aux/aux_library.h"
#include "aux/timestats.h"

#define BAM_REGION_DEFAULT_SIZE 1000

#define EMPTY_CHROM -1


/**
 *	REGION STRUCT
 */
typedef struct {
	//Reads
	bam1_t **reads;
	size_t size;
	size_t max_size;

	//Locus
	size_t init_pos;
	size_t end_pos;
	int chrom;

	//Lock
	omp_lock_t lock;
	omp_lock_t write_lock;
} bam_region_t;

/**
 * REGION OPERATIONS
 */
EXTERNC void breg_init(bam_region_t *region);
EXTERNC void breg_destroy(bam_region_t *region, int free_bam);

//EXTERNC int breg_fill(bam_region_t *region, bam_file_t *input_file);
EXTERNC void breg_write_n(bam_region_t *region, size_t n, bam_file_t *output_file);

//EXTERNC void breg_load_window(bam_region_t *region, size_t init_pos, size_t end_pos, uint8_t filters, bam_region_window_t *window);
//EXTERNC void breg_load_subwindow(bam_region_window_t *window, size_t init_pos, size_t end_pos, bam_region_window_t *out_window);

/**
 * WINDOW OPERATIONS
 */
/*EXTERNC void breg_window_init(bam_region_window_t *window);
EXTERNC void breg_window_destroy(bam_region_window_t *window);
EXTERNC void breg_window_filter(bam_region_window_t *window, uint8_t filters);
EXTERNC void breg_window_clear(bam_region_window_t *window);*/

#endif /* BAM_REGION_H_ */
