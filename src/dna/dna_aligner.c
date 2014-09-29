#include "dna_aligner.h"
//--------------------------------------------------------------------

#ifdef _VERBOSE
extern int num_dup_reads;
extern int num_total_dup_reads;
#endif

extern size_t num_mapped_reads;
extern size_t num_unmapped_reads;
extern size_t num_multihit_reads;
extern size_t num_total_mappings;
extern size_t num_unmapped_reads_by_invalid_cal;
extern size_t num_unmapped_reads_by_cigar_length;

//--------------------------------------------------------------------
// main 
//--------------------------------------------------------------------

int counters[NUM_COUNTERS];

#include "adapter.h"

void bam_sort_core_ext(int is_by_qname, const char *fn, const char *prefix, size_t max_mem, int is_stdout);

void dna_aligner(options_t *options) {
  for (int i = 0; i < NUM_COUNTERS; i++) {
    counters[i] = 0;
  }

  #ifdef _TIMING
  init_func_names();
  for (int i = 0; i < NUM_TIMING; i++) {
    func_times[i] = 0;
  }
  #endif

  // set input parameters
  char *sa_dirname = options->bwt_dirname;
  char *fastq_filename = options->in_filename;
  int batch_size = options->batch_size;
  int num_threads = options->num_cpu_threads;

  // setting output name
  int bam_format = 0;
  int len = 100;
  if (options->prefix_name) {
    len += strlen(options->prefix_name);
  }
  if (options->output_name) {
    len += strlen(options->output_name);
  }
  char out_filename[len];
  out_filename[0] = 0;
  strcat(out_filename, (options->output_name ? options->output_name : "."));
  strcat(out_filename, "/");
  if (options->prefix_name) {
    strcat(out_filename, options->prefix_name);
    strcat(out_filename, "_");
  }
  strcat(out_filename, OUTPUT_FILENAME);
  if (options->bam_format || options->realignment || options->recalibration) {
    bam_format = 1;
    strcat(out_filename, ".bam");
  } else {
    strcat(out_filename, ".sam");
  }

  // load SA index
  struct timeval stop, start;
  printf("\n");
  printf("----------------------------------------------\n");
  printf("Loading SA tables...\n");
  gettimeofday(&start, NULL);
  sa_index3_t *sa_index = sa_index3_new(sa_dirname);
  gettimeofday(&stop, NULL);
  printf("End of loading SA tables in %0.2f min. Done!!\n", 
	 ((stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0f) / 60.0f);  

  // preparing input FastQ file
  fastq_batch_reader_input_t reader_input;
  
  // preparing output BAM file
  batch_writer_input_t writer_input;
  batch_writer_input_init(out_filename, NULL, NULL, NULL, NULL, &writer_input);
  if (bam_format) {
    bam_header_t *bam_header = create_bam_header(sa_index->genome);
    writer_input.bam_file = bam_fopen_mode(out_filename, bam_header, "w");
    bam_fwrite_header(bam_header, writer_input.bam_file);
  } else {
    writer_input.bam_file = (bam_file_t *) fopen(out_filename, "w");    
    write_sam_header(sa_index->genome, (FILE *) writer_input.bam_file);
  }

  char *fq_list1 = options->in_filename, *fq_list2 = options->in_filename2;
  char token[2] = ",";
  char *ptr;
  array_list_t *files_fq1 = array_list_new(50,
					   1.25f,
					   COLLECTION_MODE_ASYNCHRONIZED);

  array_list_t *files_fq2 = array_list_new(50,
					   1.25f,
					   COLLECTION_MODE_ASYNCHRONIZED);
  int num_files1 = 0, num_files2 = 0;
  
  if (fq_list1) {
    ptr = strtok(fq_list1, token);
    array_list_insert(strdup(ptr), files_fq1);
    while( (ptr = strtok( NULL, token)) != NULL ) {
      array_list_insert(strdup(ptr), files_fq1);
    }
    num_files1 = array_list_size(files_fq1);
  }
  
  if (fq_list2) {
   ptr = strtok(fq_list2, token);
   array_list_insert(strdup(ptr), files_fq2);
   while( (ptr = strtok( NULL, token)) != NULL ) {
     array_list_insert(strdup(ptr), files_fq2);
   }    
   num_files2 = array_list_size(files_fq2);
  }
 
  
  if (fq_list2 && (num_files1 != num_files2)) {
    LOG_FATAL("Diferent number of files in paired-end/mate-pair mode");
  }
  
  char *file1, *file2;
  for (int f = 0; f < num_files1; f++) {
    file1 = array_list_get(f, files_fq1);
    
    if (num_files2) {
      file2 = array_list_get(f, files_fq2);
    } else {
      file2 = NULL;
    }
    
    fastq_batch_reader_input_init(file1, file2, 
				  options->pair_mode, 
				  batch_size, 
				  NULL, options->gzip, 
				  &reader_input);
    
    if (options->pair_mode == SINGLE_END_MODE) {
      if (options->gzip) {
	reader_input.fq_gzip_file1 = fastq_gzopen(file1);
      } else {
	reader_input.fq_file1 = fastq_fopen(file1);
      }
    } else {
      if (options->gzip) {
	reader_input.fq_gzip_file1 = fastq_gzopen(file1);
	reader_input.fq_gzip_file2 = fastq_gzopen(file2);	
      } else {
	reader_input.fq_file1 = fastq_fopen(file1);
	reader_input.fq_file2 = fastq_fopen(file2);
      }
    }
    
    //--------------------------------------------------------------------------------------
    // workflow management
    //
    sa_wf_batch_t *wf_batch = sa_wf_batch_new(options, (void *)sa_index, &writer_input, NULL, NULL);
    sa_wf_input_t *wf_input = sa_wf_input_new(bam_format, &reader_input, wf_batch);

    
    // create and initialize workflow
    workflow_t *wf = workflow_new();
    
    workflow_stage_function_t stage_functions[1];
    char *stage_labels[1] = {"SA mapper"};
    if (options->pair_mode == SINGLE_END_MODE) {
      stage_functions[0] = sa_single_mapper;
    } else {
      stage_functions[0] = sa_pair_mapper;
    }
    workflow_set_stages(1, stage_functions, stage_labels, wf);
    
    // optional producer and consumer functions
    workflow_set_producer((workflow_producer_function_t *)sa_fq_reader, "FastQ reader", wf);
    if (bam_format) {
      workflow_set_consumer((workflow_consumer_function_t *)sa_bam_writer, "BAM writer", wf);
    } else {
      workflow_set_consumer((workflow_consumer_function_t *)sa_sam_writer, "SAM writer", wf);
    }
    
    printf("----------------------------------------------\n");
    printf("Starting mapping...\n");
    gettimeofday(&start, NULL);
    workflow_run_with(num_threads, wf_input, wf);
    gettimeofday(&stop, NULL);
    printf("End of mapping in %0.2f min. Done!!\n", 
	   ((stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0f)/60.0f);  
    
    printf("----------------------------------------------\n");
    printf("Output file        : %s\n", out_filename);
    printf("\n");
    printf("Num. reads         : %lu\nNum. mapped reads  : %lu (%0.2f %%)\nNum. unmapped reads: %lu (%0.2f %%)\n",
	   num_mapped_reads + num_unmapped_reads,
	   num_mapped_reads, 100.0f * num_mapped_reads / (num_mapped_reads + num_unmapped_reads),
	   num_unmapped_reads, 100.0f * num_unmapped_reads / (num_mapped_reads + num_unmapped_reads));
    printf("\n");
    printf("Num. mappings      : %lu\n", num_total_mappings);
    printf("Num. multihit reads: %lu\n", num_multihit_reads);
    printf("----------------------------------------------\n");


  #ifdef _TIMING
    char func_name[1024];
    double total_func_times = 0;
    for (int i = 0; i < NUM_TIMING; i++) {
      if (i != FUNC_SEARCH_PREFIX && i != FUNC_SEARCH_SA 
	  && i < FUNC_INIT_CALS_FROM_SUFFIXES || i > FUNC_CAL_MNG_INSERT) {
	total_func_times += func_times[i];
      }
    }
    printf("Timing in seconds:\n");
    for (int i = 0; i < NUM_TIMING; i++) {
      if (i == FUNC_SEARCH_PREFIX || i == FUNC_SEARCH_SA ||
	  (i >= FUNC_INIT_CALS_FROM_SUFFIXES && i <= FUNC_CAL_MNG_INSERT)) {
      printf("\t");
      }
      printf("\t%0.2f %%\t%0.4f\tof %0.4f\t%s\n", 
	     100.0 * func_times[i] / total_func_times, func_times[i], total_func_times, func_names[i]);
    }
  #endif
    
    // free memory
    sa_wf_input_free(wf_input);
    sa_wf_batch_free(wf_batch);
    workflow_free(wf);

    //closing files
    if (options->gzip) {
      if (options->pair_mode == SINGLE_END_MODE) {
	fastq_gzclose(reader_input.fq_gzip_file1);
      } else {
	fastq_gzclose(reader_input.fq_gzip_file1);
	fastq_gzclose(reader_input.fq_gzip_file2);
      }
    } else {
      if (options->pair_mode == SINGLE_END_MODE) {
	fastq_fclose(reader_input.fq_file1);
      } else {
	fastq_fclose(reader_input.fq_file1);
	fastq_fclose(reader_input.fq_file2);
      }
    }
    
    //
    // end of workflow management
    //--------------------------------------------------------------------------------------
  }

  // free memory
  array_list_free(files_fq1, (void *) free);
  array_list_free(files_fq2, (void *) free);
  if (sa_index) sa_index3_free(sa_index);
  
  //closing files
  if (bam_format) {
    bam_fclose(writer_input.bam_file);
  } else {
    fclose((FILE *) writer_input.bam_file);
  }

  // post-processing: realignment and recalibration
  if (options->realignment || options->recalibration) {
    printf("-----------------------------------\n");
    #ifdef D_TIME_DEBUG
    // Initialize stats
    if(time_new_stats(20, &TIME_GLOBAL_STATS))  {
      printf("ERROR: FAILED TO INITIALIZE TIME STATS\n");
    } else {
      printf("STATISTICS ACTIVATED\n");
    }
    #endif
  }

  char *aux = out_filename;
  char realig_filename[len], recal_filename[len];
  if (options->realignment) {
    // first, sort
    printf("-----------------------------------\nSorting before re-aligning...\n");
    char sorted_filename[len];
    sorted_filename[0] = 0;
    strcat(sorted_filename, (options->output_name ? options->output_name : "."));
    strcat(sorted_filename, "/");
    if (options->prefix_name) {
      strcat(sorted_filename, options->prefix_name);
      strcat(sorted_filename, "_");
    }
    strcat(sorted_filename, "sorted_");
    strcat(sorted_filename, OUTPUT_FILENAME);

    // run sort
    bam_sort_core_ext(0, out_filename, sorted_filename, 500000000, 0);
    printf("Done!\n");

    // and then, re-align
    strcat(sorted_filename, ".bam");
    printf("-----------------------------------\nRealigning...\n");
    realig_filename[0] = 0;
    strcat(realig_filename, (options->output_name ? options->output_name : "."));
    strcat(realig_filename, "/");
    if (options->prefix_name) {
      strcat(realig_filename, options->prefix_name);
      strcat(realig_filename, "_");
    }
    strcat(realig_filename, "realigned_");
    strcat(realig_filename, OUTPUT_FILENAME);
    strcat(realig_filename, ".bam");

    char ref_filename[strlen(sa_dirname) + 100];
    ref_filename[0] = 0;
    strcpy(ref_filename, sa_dirname);
    strcat(ref_filename, "/dna_compression.bin");

    printf("sorted_filename = %s\n", sorted_filename);
    printf("ref_filename = %s\n", ref_filename);
    printf("realig_filename = %s\n", realig_filename);

    alig_bam_file(sorted_filename, ref_filename, realig_filename, NULL);
    aux = realig_filename;
    printf("Realigned file     : %s\n", realig_filename);
  }


  if (options->recalibration) {
    recal_filename[0] = 0;
    strcat(recal_filename, (options->output_name ? options->output_name : "."));
    strcat(recal_filename, "/");
    if (options->prefix_name) {
      strcat(recal_filename, options->prefix_name);
      strcat(recal_filename, "_");
    }
    if (options->realignment) {
      strcat(recal_filename, "recalibrated_realigned_");
      strcat(recal_filename, OUTPUT_FILENAME);
      strcat(recal_filename, ".bam");
    } else {
      strcat(recal_filename, "recalibrated_");
      strcat(recal_filename, OUTPUT_FILENAME);
      strcat(recal_filename, ".bam");
    }

    char ref_filename[strlen(sa_dirname) + 100];
    ref_filename[0] = 0;
    strcpy(ref_filename, sa_dirname);
    strcat(ref_filename, "/dna_compression.bin");

    recal_bam_file(RECALIBRATE_COLLECT | RECALIBRATE_RECALIBRATE, aux, ref_filename, 
		   NULL, NULL, recal_filename, 500, NULL);
    printf("Recalibrated file  : %s\n", recal_filename);
  }


  #ifdef _VERBOSE
  printf("*********> num_dup_reads = %i, num_total_dup_reads = %i\n", 
	 num_dup_reads, num_total_dup_reads);
  #endif
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
