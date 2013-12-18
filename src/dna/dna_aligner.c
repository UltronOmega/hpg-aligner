#include "dna_aligner.h"
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// main 
//--------------------------------------------------------------------

void dna_aligner(options_t *options) {
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
  int len = 100;
  if (options->prefix_name) {
    len += strlen(options->prefix_name);
  }
  if (options->output_name) {
    len += strlen(options->output_name);
  }
  char sam_filename[len];
  sam_filename[0] = 0;
  strcat(sam_filename, (options->output_name ? options->output_name : "."));
  strcat(sam_filename, "/");
  if (options->prefix_name) {
    strcat(sam_filename, options->prefix_name);
    strcat(sam_filename, "_");
  }
  strcat(sam_filename, "out.sam");

  // load SA index
  struct timeval stop, start;
  printf("\n");
  printf("Loading SA tables...\n");
  gettimeofday(&start, NULL);
  sa_index3_t *sa_index = sa_index3_new(sa_dirname);
  gettimeofday(&stop, NULL);
  sa_index3_display(sa_index);
  printf("End of loading SA tables in %0.2f s. Done!!\n", 
	 (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0f);  

  // preparing input FastQ file
  fastq_batch_reader_input_t reader_input;
  fastq_batch_reader_input_init(fastq_filename, NULL, 
				0, batch_size, 
				NULL, &reader_input);
  
  reader_input.fq_file1 = fastq_fopen(fastq_filename);
  reader_input.fq_file2 = NULL;
  
  // preparing output BAM file
  batch_writer_input_t writer_input;
  batch_writer_input_init(sam_filename, NULL, NULL, NULL, NULL, &writer_input);
  
  writer_input.bam_file = (bam_file_t *) fopen(sam_filename, "w");    
  write_sam_header(sa_index->genome, (FILE *) writer_input.bam_file);
  
  //--------------------------------------------------------------------------------------
  // workflow management
  //
  sa_wf_batch_t *wf_batch = sa_wf_batch_new((void *)sa_index, &writer_input, NULL);
  sa_wf_input_t *wf_input = sa_wf_input_new(&reader_input, wf_batch);

  // create and initialize workflow
  workflow_t *wf = workflow_new();
  
  workflow_stage_function_t stage_functions[] = {sa_mapper};
  char *stage_labels[] = {"SA mapper"};
  workflow_set_stages(1, &stage_functions, stage_labels, wf);
  
  // optional producer and consumer functions
  workflow_set_producer(sa_fq_reader, "FastQ reader", wf);
  workflow_set_consumer(sa_sam_writer, "SAM writer", wf);

  workflow_run_with(num_threads, wf_input, wf);
  
  printf("----------------------------------------------\n");
  workflow_display_timing(wf);
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

  //  printf("Total num. mappings: %u\n", total_num_mappings);

  // free memory
  workflow_free(wf);
  sa_wf_input_free(wf_input);
  sa_wf_batch_free(wf_batch);
  //
  // end of workflow management
  //--------------------------------------------------------------------------------------

  // free memory
  if (sa_index) sa_index3_free(sa_index);
  
  //closing files
  fastq_fclose(reader_input.fq_file1);
  fclose((FILE *) writer_input.bam_file);

  #ifdef _VERBOSE
  printf("*********> num_dup_reads = %i, num_total_dup_reads = %i\n", 
	 num_dup_reads, num_total_dup_reads);
  #endif

}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
