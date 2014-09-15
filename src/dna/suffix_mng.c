#include "suffix_mng.h"

//--------------------------------------------------------------------
// suffix manager
//--------------------------------------------------------------------

suffix_mng_t *suffix_mng_new(sa_genome3_t *genome) {
  suffix_mng_t *p = (suffix_mng_t *) calloc(1, sizeof(suffix_mng_t));

  int num_chroms = genome->num_chroms;

  char *name;
  Container *subject = (Container *) malloc(sizeof(Container));
  bl_containerInit(subject, num_chroms, sizeof(char *));

  linked_list_t **suffix_lists = (linked_list_t **) malloc (sizeof(linked_list_t *) * num_chroms);
  for (int i = 0; i < num_chroms; i++) {
    suffix_lists[i] = linked_list_new(COLLECTION_MODE_ASYNCHRONIZED);

    name = calloc(64, sizeof(char));
    sprintf(name, "%i", i);
    bl_containerAdd(subject, &name);
  }

  p->num_seeds = 0;
  p->num_chroms = num_chroms;
  p->subject = subject;
  p->suffix_lists = suffix_lists;

  return p;
}

//--------------------------------------------------------------------

void suffix_mng_free(suffix_mng_t *p) {
  if (p) {
    if (p->suffix_lists) {
      for (int i = 0; i < p->num_chroms; i++) {
	if (p->suffix_lists[i]) {
	  linked_list_free(p->suffix_lists[i], (void *)seed_free);
	}
      }
      free(p->suffix_lists);
    }
    if (p->subject){
      for (int i = 0; i < bl_containerSize(p->subject); i++) {
	free(*(char **) bl_containerGet(p->subject, i));
      }
      bl_containerDestruct(p->subject, NULL);
      free(p->subject);
    }
    free(p);
  }
}

//--------------------------------------------------------------------

void suffix_mng_clear(suffix_mng_t *p) {
  if (p) {
    p->num_seeds = 0;
    if (p->suffix_lists) {
      for (unsigned int i = 0; i < p->num_chroms; i++) {
	if (p->suffix_lists[i]) {
	  linked_list_clear(p->suffix_lists[i], (void *)seed_free);
	}
      }
    }
  }
}

//--------------------------------------------------------------------

void suffix_mng_update(int chrom, size_t read_start, size_t read_end, 
		       size_t genome_start, size_t genome_end, 
		       suffix_mng_t *p) {

  if (!p) return;
  if (!p->suffix_lists) return;

  int found_cal = 0;
  linked_list_t *suffix_list = p->suffix_lists[chrom];
  if (!suffix_list) return;

  seed_t *seed;

  if (linked_list_size(suffix_list) <= 0) {
    // list is empty, insert and return
    seed = seed_new(read_start, read_end, genome_start, genome_end);
    seed->chromosome_id = chrom;
    linked_list_insert(seed, suffix_list);
    p->num_seeds++;
    return;
  }
  
  linked_list_iterator_t* itr = linked_list_iterator_new(suffix_list);
  seed = (seed_t *) linked_list_iterator_curr(itr);
  while (seed != NULL) {
    // if it's included then return
    if (seed->chromosome_id == chrom && 
	seed->read_start <= read_start && seed->read_end >= read_end &&
	seed->genome_start <= genome_start && seed->genome_end >= genome_end) {
      
      // free memory
      linked_list_iterator_free(itr);
      return;
    } 
    
    // if it's previous then insert and return
    if (genome_start < seed->genome_start) {
      seed = seed_new(read_start, read_end, genome_start, genome_end);
      seed->chromosome_id = chrom;
      linked_list_iterator_insert(seed, itr);
      linked_list_iterator_prev(itr);
      p->num_seeds++;

      // free memory
      linked_list_iterator_free(itr);
      return;
    }
    
    //continue loop...
    linked_list_iterator_next(itr);
    seed = linked_list_iterator_curr(itr);
  }
  // insert at the last position
  seed = seed_new(read_start, read_end, genome_start, genome_end);
  seed->chromosome_id = chrom;
  linked_list_insert_last(seed, suffix_list);
  p->num_seeds++;

  // free memory
  linked_list_iterator_free(itr);
}

//--------------------------------------------------------------------

void extend_to_left(seed_t *seed, int max_length, 
		    seed_cal_t *cal, sa_index3_t *sa_index,
		    alig_out_t *alig_out) {
  cigar_init(&alig_out->cigar);

  int chrom = cal->chromosome_id;

  size_t r_end = seed->read_start - 1;
  size_t r_start = r_end - max_length;
  size_t r_len = max_length;
  char *r_seq = (cal->strand ? cal->read->revcomp : cal->read->sequence);
  
  size_t g_len = r_len + 10;
  size_t g_end = seed->genome_start - 1;
  size_t g_start = g_end - g_len;
  char *g_seq = &sa_index->genome->S[g_start + sa_index->genome->chrom_offsets[chrom] + 1];
  
  float score = doscadfun_inv(r_seq, r_len, g_seq, g_len, MISMATCH_PERC,
			      alig_out);
}

void extend_to_right(seed_t *seed, int max_length, 
		     seed_cal_t *cal, sa_index3_t *sa_index,
		     alig_out_t *alig_out) {
  
  cigar_init(&alig_out->cigar);

  int chrom = cal->chromosome_id;

  size_t r_start = seed->read_end + 1;
  size_t r_end = seed->read_end + max_length;
  size_t r_len = max_length;
  char *r_seq = (cal->strand ? cal->read->revcomp : cal->read->sequence);
  
  size_t g_len = r_len + 10;
  size_t g_start = seed->genome_end + 1;
  size_t g_end = g_start + g_len;

  char *g_seq = &sa_index->genome->S[g_start + sa_index->genome->chrom_offsets[chrom]];
  
  float score = doscadfun(&r_seq[r_start], r_len, g_seq, g_len, MISMATCH_PERC,
			  alig_out);
}


void update_seed_left(alig_out_t *alig_out, seed_t *seed, seed_cal_t *cal) {
  if (alig_out->match > 0) {
  //if (score > 0 || (alig_out.map_len1 + suffix_len) > 20) {

    // update seed
    seed->num_mismatches += alig_out->mismatch;
    seed->num_open_gaps += alig_out->gap_open;
    seed->num_extend_gaps += alig_out->gap_extend;
    
    seed->read_start -= alig_out->map_len1;
    seed->genome_start -= alig_out->map_len2;
    /*
    // update cal
    cal->num_mismatches += alig_out->mismatch;
    cal->num_open_gaps += alig_out->gap_open;
    cal->num_extend_gaps += alig_out->gap_extend;
    cal->read_area += alig_out->map_len1;
    */
    /*
    // if there's a mini-gap then try to fill the mini-gap
    r_start = seed->read_start - alig_out->map_len1;
    if (r_start > 0 && r_start < 5) {
      alig_out.map_len1 += r_start;
      alig_out.map_len2 += r_start;

      cigar_append_op(r_start, 'S', &cigar);
    }
    */
    // update cigar with the sw output
    if (alig_out->cigar.num_ops > 0) {
      cigar_t cigar;
      cigar_init(&cigar);

      cigar_concat(&seed->cigar, &alig_out->cigar);
      cigar_init(&seed->cigar);
      cigar_copy(&seed->cigar, &alig_out->cigar);
    }    
  }
}


void update_seed_right(alig_out_t *alig_out, seed_t *seed, seed_cal_t *cal) {
  if (alig_out->match > 0) {
  //if (score > 0 || (alig_out.map_len1 + suffix_len) > 20) {

    // update seed
    seed->num_mismatches += alig_out->mismatch;
    seed->num_open_gaps += alig_out->gap_open;
    seed->num_extend_gaps += alig_out->gap_extend;
    
    seed->read_end += alig_out->map_len1;
    seed->genome_end += alig_out->map_len2;

    /*
    // update cal
    cal->num_mismatches += alig_out->mismatch;
    cal->num_open_gaps += alig_out->gap_open;
    cal->num_extend_gaps += alig_out->gap_extend;
    cal->read_area += alig_out->map_len1;
    */

    /*
    // if there's a mini-gap then try to fill the mini-gap
    r_start = seed->read_start - alig_out->map_len1;
    if (r_start > 0 && r_start < 5) {
      alig_out.map_len1 += r_start;
      alig_out.map_len2 += r_start;

      cigar_append_op(r_start, 'S', &cigar);
    }
    */
    // update cigar with the sw output
    if (alig_out->cigar.num_ops > 0) {
      cigar_concat(&alig_out->cigar, &seed->cigar);
    }    
  }
}

void extend_seeds(seed_cal_t *cal, sa_index3_t *sa_index) {
  size_t gap_read_start, gap_read_end;
  size_t genome_pos, gap_genome_start, gap_genome_end;
  int chrom, gap_read_len, gap_genome_len;

  linked_list_item_t *item;
  seed_t *prev_seed, *seed;

  int read_area, genome_area, num_seeds, num_matches, num_mismatches;
  char *seq, *r_seq, *g_seq;
  alig_out_t alig_out;

  fastq_read_t *read = cal->read;

  chrom = cal->chromosome_id;
  /*
    if (cal->chromosome_id != 0 || cal->start != 16141124) {
    continue;
    }
  */
  seq = (cal->strand ? read->revcomp : read->sequence);
  
  //printf("---> fill_seed_gaps:\n");
  //seed_cal_print(cal);
  
  num_seeds = cal->seed_list->size;
  if (num_seeds <= 0) return;
  
  // first seed
  seed = linked_list_get_first(cal->seed_list);
  cal->start = seed->genome_start;
  if (seed->read_start > 0) {
    // extend to left
    //printf("\nextending initial seed to left\n");
    //seed_cal_print(cal);
    extend_to_left(seed, seed->read_start, cal, sa_index, &alig_out);
    update_seed_left(&alig_out, seed, cal);
    cal->start = seed->genome_start;
    //printf("------> cigar = %s\n", cigar_to_string(&alig_out.cigar));
    //alig_out_print(&alig_out);
    cigar_clean(&alig_out.cigar);
  }
  
  // seeds at the middle positions
  prev_seed = seed;
  for (item = cal->seed_list->first->next; item != NULL; item = item->next) {
    seed = item->item;
    
    read_area = 0;
    genome_area = 0;

    // gap in read
    gap_read_start = prev_seed->read_end + 1;
    gap_read_end = seed->read_start - 1;
    gap_read_len = abs(gap_read_end - gap_read_start + 1);    

    // gap in genome
    gap_genome_start = prev_seed->genome_end + 1;
    gap_genome_end = seed->genome_start - 1;
    gap_genome_len = abs(gap_genome_end - gap_genome_start + 1);

    //printf("gap lengths (read, genome) = (%i, %i)\n", gap_read_len, gap_genome_len);

    if (gap_read_len > 0) {
      // extend previous seed to right
      extend_to_right(prev_seed, gap_read_len, cal, sa_index, &alig_out);
      //alig_out_print(&alig_out);
      if (alig_out.match) {
	update_seed_right(&alig_out, prev_seed, cal);
	read_area = alig_out.map_len1;
	genome_area = alig_out.map_len2;
	
	// gap in read
	gap_read_start = prev_seed->read_end + 1;
	gap_read_end = seed->read_start - 1;
	gap_read_len = abs(gap_read_end - gap_read_start + 1);    
	
	// gap in genome
	gap_genome_start = prev_seed->genome_end + 1;
	gap_genome_end = seed->genome_start - 1;
	gap_genome_len = abs(gap_genome_end - gap_genome_start + 1);

	//printf("after extend to right, gap lengths (read, genome) = (%i, %i)\n", gap_read_len, gap_genome_len);
      }
    }

    if (gap_read_len > 0) {
      // extend current seed to left
      extend_to_left(seed, gap_read_len, cal, sa_index, &alig_out);
      //alig_out_print(&alig_out);
      if (alig_out.match) {
	update_seed_left(&alig_out, seed, cal);
	read_area += alig_out.map_len1;
	genome_area += alig_out.map_len2;
      }
    }

    //seed_cal_print(cal);
    
    prev_seed = seed;
  }
  
  // last seed
  cal->end = seed->genome_end;
  if (seed->read_end < read->length - 1) {
    // extend to right
    //printf("\nextending final seed to left\n");
    //seed_cal_print(cal);
    extend_to_right(seed, read->length - seed->read_end - 1, cal, sa_index, &alig_out);
    update_seed_right(&alig_out, seed, cal);
    cal->end = seed->genome_end;
    //printf("------> cigar = %s\n", cigar_to_string(&alig_out.cigar));
    //alig_out_print(&alig_out);
    cigar_clean(&alig_out.cigar);
  }
}

//--------------------------------------------------------------------

void suffix_mng_create_cals(fastq_read_t *read, int min_area, int strand, 
			    sa_index3_t *sa_index, array_list_t *cal_list,
			    suffix_mng_t *p) {

  if (!p) return;
  if (!p->suffix_lists) return;

  if (p->num_seeds <= 0) return;

  int read_area, chrom;
  seed_t *seed;
  seed_cal_t *cal;
  linked_list_t *seed_list;
  claspinfo_t info;
  bl_claspinfoInit(&info);

  // initialization
  info.fragments = (Container *) malloc(sizeof(Container));
  bl_containerInit(info.fragments, p->num_seeds, sizeof(slmatch_t));

  info.subject = p->subject;

  slmatch_t frag;
  linked_list_t *suffix_list;
  for (unsigned int i = 0; i < p->num_chroms; i++) {
    suffix_list = p->suffix_lists[i];
    if (suffix_list) {
      for (linked_list_item_t *item = suffix_list->first; 
	   item != NULL; 
	   item = item->next) {

	seed = item->item;

	bl_slmatchInit(&frag, 0);
	frag.i = seed->read_start;
	frag.j = seed->read_end - seed->read_start + 1;
	frag.p = seed->genome_start;
	frag.q = seed->genome_end - seed->genome_start + 1;
	frag.scr = seed->genome_end - seed->genome_start + 1;
	frag.subject = seed->chromosome_id;
	bl_containerAdd(info.fragments, &frag);
      }
    }
  }

  // sort fragments
  qsort(info.fragments->contspace, bl_containerSize(info.fragments),
	sizeof(slmatch_t), cmp_slmatch_qsort);
  int begin = 0;
  for (int i = 1; i <= bl_containerSize(info.fragments); i++){
    // end of fragments list or different database sequence 
    // --> process fragment[begin]...fragment[i-1], write output
    // and free chains (less memory consumption with large input files)
    if (i == bl_containerSize(info.fragments) ||
	((slmatch_t *) bl_containerGet(info.fragments, begin))->subject !=
	((slmatch_t *) bl_containerGet(info.fragments, i))->subject){
      //fprintf(info.dev, "%d\t%d\n", begin, i-begin);
      if (info.chainmode == SOP){
	// only use chaining without clustering if no ids are specified
	bl_slClusterSop((slmatch_t *) info.fragments->contspace + begin, i - begin,
			info.epsilon, info.lambda, info.maxgap);
      }
      else {    
	bl_slClusterLin((slmatch_t *) info.fragments->contspace + begin, i - begin,
			info.epsilon, info.lambda, info.maxgap);
      }
      
      for (int j = begin; j < i; j++) {


	slmatch_t *match = (slmatch_t *) bl_containerGet(info.fragments, j);

	if (match->chain) {
	  slchain_t *chain = (slchain_t *) match->chain;

	  if (chain->scr >= info.minscore &&
	      bl_containerSize(chain->matches) >= info.minfrag) {
	    /*
	    fprintf(info.dev, "C\t");
	    fprintf(info.dev, "%s\t", *(char **) bl_containerGet(info.subject, chain->subject));
	    fprintf(info.dev, "%d\t%d\t%d\t%d\t%.3f\n", chain->i,
		    chain->i + chain->j - 1, chain->p,
	    	    chain->p + chain->q - 1, chain->scr);
	    */

	    chrom = atoi(*(char **) bl_containerGet(info.subject, chain->subject));
	    
	    read_area = 0;
	    seed_list = linked_list_new(COLLECTION_MODE_ASYNCHRONIZED);
	    
	    for (int k = 0; k < bl_containerSize(chain->matches); k++){
	      slmatch_t *frag = *(slmatch_t **) bl_containerGet(chain->matches, k);
	      /*
	      fprintf(info.dev, "------> F\t");
	      fprintf(info.dev, "%s\t",
		      *(char **) bl_containerGet(info.subject, frag->subject));
	      fprintf(info.dev, "%d\t%d\t%d\t%d\t%.3f\n", frag->i,
		      frag->i + frag->j - 1, frag->p, frag->p + frag->q - 1,
		      frag->scr);
	      */
	      seed = seed_new(frag->i, frag->i + frag->j - 1, frag->p, frag->p + frag->q - 1);
	      seed->chromosome_id = chrom;
	      seed->strand = strand;
	      read_area += frag->j;
	      cigar_append_op(frag->j, '=', &seed->cigar);
	      
	      linked_list_insert_last(seed, seed_list);
	    }

	    // extend seeds	    
	    cal = seed_cal_new(chrom, strand, chain->p, chain->p + chain->q - 1, seed_list);
	    cal->read = read;
	    //cal->read_area = read_area;
	    //cal->score = chain->scr;
	    //cal->num_mismatches = 0;
	    //printf("---------------------------------------------\n");
	    //seed_cal_print(cal);
	    //printf("after extending...\n");
	    extend_seeds(cal, sa_index);
	    seed_cal_update_info(cal);
	    //seed_cal_print(cal);
	    //printf("---------------------------------------------\n");

	    if (cal->read_area >= min_area) {
	      //printf("-----> yes, cal->read_area = %i >= %i = min_area\n", cal->read_area, min_area);
	      array_list_insert(cal, cal_list);
	    } else {
	      //printf("-----> sorry but cal->read_area = %i < %i = min_area\n", cal->read_area, min_area);
	      seed_cal_free(cal);
	      //linked_list_free(seed_list, (void *)seed_free);
	    }
	  }

	  bl_slchainDestruct(chain);
	  free(chain);
	  match->chain = NULL;
	}

	/*
	// output matches (if desired)
	if (info.outputm){
	  fprintf(info.dev, "M\t");
	  if (!info.outputorig){
	    if (info.idcol != NULL){
	      fprintf(info.dev, "%s\t",
		      *(char **) bl_containerGet(info.subject, match->subject));
	    }
	    fprintf(info.dev, "%d\t%d\t%d\t%d\t%.3f\n", match->i,
		    match->i + match->j - 1, match->p,
		    match->p + match->q - 1, match->scr);
	  }
	  // output in original format as input
	  else {
	    fprintf(info.dev, "%s\n", *(char **) bl_containerGet(info.lines, j));
	  }
	}
	if (match->chain){
	  slchain_t *chain = (slchain_t *) match->chain;
	  if (info.outputc && chain->scr >= info.minscore &&
	      bl_containerSize(chain->matches) >= info.minfrag){
	    fprintf(info.dev, "C\t");
	    if (1) { //info.idcol != NULL){
	      fprintf(info.dev, "%s\t", *(char **) bl_containerGet(info.subject, chain->subject));
	    }
	    fprintf(info.dev, "%d\t%d\t%d\t%d\t%.3f\n", chain->i,
		    chain->i + chain->j - 1, chain->p,
		    chain->p + chain->q - 1, chain->scr);
	  }
	  // output chains and fragments (if requested)
	  info.outputf = 1;
	  if (info.outputf && chain->scr >= info.minscore &&
	      bl_containerSize(chain->matches) >= info.minfrag){
	    for (int k = 0; k < bl_containerSize(chain->matches); k++){
	      slmatch_t *frag = *(slmatch_t **)
		bl_containerGet(chain->matches, k);
	      fprintf(info.dev, "F\t");
	      if (!info.outputorig){
		if (1) { //info.idcol != NULL){
		  fprintf(info.dev, "%s\t",
			  *(char **) bl_containerGet(info.subject, frag->subject));
		}
		fprintf(info.dev, "%d\t%d\t%d\t%d\t%.3f\n", frag->i,
			frag->i + frag->j - 1, frag->p, frag->p + frag->q - 1,
			frag->scr);
	      }
	      // output in original format as input
	      else {
		fprintf(info.dev, "%s\n",
			*(char **) bl_containerGet(info.lines, frag->idx));
	      }
	    }
	  }
	  bl_slchainDestruct(chain);
	  free(chain);
	  match->chain = NULL;
	} // END OF if (frag->chain)
*/
      }  // END OF for (j = begin; j < i; j++)
      begin = i;
    } // END OF  if (i == bl_containerSize(info.fragments) ||
  } // END OF for (i = 1; i <= bl_containerSize(info.fragments); i++)

  // destruct everything
  info.subject = NULL;
  bl_claspinfoDestruct(&info);

  // finally, clear suffix manager
  suffix_mng_clear(p);

  /*
  printf("*********************************************\n");
  printf("after creating cals:\n");
  for (int kk = 0; kk < array_list_size(cal_list); kk++) { seed_cal_print(array_list_get(kk, cal_list)); }
  printf("*********************************************\n");
  */
}

//--------------------------------------------------------------------

void suffix_mng_search_read_cals(fastq_read_t *read, int num_seeds, 
				 sa_index3_t *sa_index, array_list_t *cal_list, 
				 suffix_mng_t *suffix_mng) {
  int max_suffixes = MAX_NUM_SUFFIXES;
  int chrom, num_suffixes;
  size_t suffix_len;
  size_t low, high, r_start_suf, r_end_suf, g_start_suf, g_end_suf;

  int read_pos, read_inc = read->length / num_seeds;
  if (read_inc < sa_index->k_value / 2) {
    read_inc = sa_index->k_value / 2;
  }

  // fill in the CAL manager structure
  int read_end_pos = read->length - sa_index->k_value;
  int extra_seed = (read->length - sa_index->k_value) % read_inc;

  //    memset(saved_pos, 0, sizeof(saved_pos));

  // first step, searching mappings in both strands
  // distance between seeds >= prefix value (sa_index->k_value)
  char *r_seq = read->sequence;
  for (int strand = 0; strand < 2; strand++) {
    for (read_pos = 0; read_pos < read_end_pos; read_pos += read_inc)  {	
      num_suffixes = search_suffix(&r_seq[read_pos], sa_index->k_value, 
				   max_suffixes, sa_index, 
				   &low, &high, &suffix_len
                                   #ifdef _TIMING
				   , &prefix_time, &suffix_time
                                   #endif
				   );
      if (num_suffixes < max_suffixes && suffix_len) {
	for (size_t suff = low; suff <= high; suff++) {
	  chrom = sa_index->CHROM[suff];
	  // extend suffix to right side
	  r_start_suf = read_pos;
	  r_end_suf = r_start_suf + suffix_len - 1;
	  
	  g_start_suf = sa_index->SA[suff] - sa_index->genome->chrom_offsets[chrom];
	  g_end_suf = g_start_suf + suffix_len - 1;
	  
	  suffix_mng_update(chrom, r_start_suf, r_end_suf, g_start_suf, g_end_suf, suffix_mng);
	}
      }
    } // end of for read_pos
    
    if (suffix_len != read->length && extra_seed) {
      read_pos = read->length - sa_index->k_value;
      num_suffixes = search_suffix(&r_seq[read_pos], sa_index->k_value, 
				   max_suffixes, sa_index, 
				   &low, &high, &suffix_len
                                   #ifdef _TIMING
				   , &prefix_time, &suffix_time
                                   #endif
				   );
      if (num_suffixes < max_suffixes && suffix_len) {
	for (size_t suff = low; suff <= high; suff++) {
	  chrom = sa_index->CHROM[suff];
	  // extend suffix to right side
	  r_start_suf = read_pos;
	  r_end_suf = r_start_suf + suffix_len - 1;
	  
	  g_start_suf = sa_index->SA[suff] - sa_index->genome->chrom_offsets[chrom];
	  g_end_suf = g_start_suf + suffix_len - 1;
	  
	  suffix_mng_update(chrom, r_start_suf, r_end_suf, g_start_suf, g_end_suf, suffix_mng);
	}
      }
    }

    // using suffix manager instead of the previous cal manager
    suffix_mng_create_cals(read, read->length / 3, strand, 
			   sa_index, cal_list, suffix_mng);
    
    // next, - strand
    r_seq = read->revcomp;
  } // end of for strand
}

//--------------------------------------------------------------------

void suffix_mng_search_read_cals_by_region(fastq_read_t *read, int num_seeds, 
					   sa_index3_t *sa_index, 
					   int strand, int chromosome, 
					   size_t start, size_t end, 
					   array_list_t *cal_list, 
					   suffix_mng_t *suffix_mng) {
  int max_suffixes = MAX_NUM_SUFFIXES;
  int chrom, num_prefixes, num_suffixes, suffix_len;
  size_t low, high, r_start_suf, r_end_suf, g_start_suf, g_end_suf;

  int read_pos, read_inc = read->length / num_seeds;
  if (read_inc < sa_index->k_value / 2) {
    read_inc = sa_index->k_value / 2;
  }

  // fill in the CAL manager structure
  int read_end_pos = read->length - sa_index->k_value;
  int extra_seed = (read->length - sa_index->k_value) % read_inc;

  //  read_inc = 1;
  //  extra_seed = 0;

  // first step, searching mappings in both strands
  // distance between seeds >= prefix value (sa_index->k_value)
  char *r_seq = strand == 0 ? read->sequence : read->revcomp;
  for (read_pos = 0; read_pos < read_end_pos; read_pos += read_inc)  {	
    num_prefixes = search_prefix(&r_seq[read_pos], &low, &high, sa_index, 0);
    num_suffixes = num_prefixes;
    suffix_len = num_suffixes > 0 ? sa_index->k_value : 0;
    /*
    num_suffixes = search_suffix(&r_seq[read_pos], sa_index->k_value, 
				 max_suffixes, sa_index, 
				 &low, &high, &suffix_len
                                 #ifdef _TIMING
				 , &prefix_time, &suffix_time
                                 #endif
				 );
    */
    //printf("%c read_pos = %i, (num_suffixes = %i, suffix_len = %i)\n",
    //	   (strand == 0 ? '+' : '-'), read_pos, num_suffixes, suffix_len);
    if (num_suffixes > 0) {
      for (size_t suff = low; suff <= high; suff++) {
	chrom = sa_index->CHROM[suff];
	if (chrom == chromosome) {
	  // extend suffix to right side
	  r_start_suf = read_pos;
	  r_end_suf = r_start_suf + suffix_len - 1;
	  
	  g_start_suf = sa_index->SA[suff] - sa_index->genome->chrom_offsets[chrom];
	  g_end_suf = g_start_suf + suffix_len - 1;
	  
	  //printf("region (chrom, start, end) = (%i, %lu, %lu), read_pos = %i, suffix = (%i, %lu, %lu)\n", 
	  //	 chromosome, start, end, read_pos, chrom, g_start_suf, g_end_suf);
	  
	  if (start <= g_start_suf && end >= g_end_suf) {
	    //printf("\t\tg_start_suf, g_end_suf) = (%lu, %lu)\n", g_start_suf, g_end_suf);
	    suffix_mng_update(chrom, r_start_suf, r_end_suf, g_start_suf, g_end_suf, suffix_mng);
	  }
	}
      }
    }
  } // end of for read_pos
    
  if (suffix_len != read->length && extra_seed) {
    read_pos = read->length - sa_index->k_value;
    num_prefixes = search_prefix(&r_seq[read_pos], &low, &high, sa_index, 0);
    num_suffixes = num_prefixes;
    suffix_len = num_suffixes > 0 ? sa_index->k_value : 0;
    /*
    num_suffixes = search_suffix(&r_seq[read_pos], sa_index->k_value, 
				 max_suffixes, sa_index, 
				 &low, &high, &suffix_len
                                 #ifdef _TIMING
				 , &prefix_time, &suffix_time
                                 #endif
				 );
    */
    //printf("%c read_pos = %i, (num_suffixes = %i, suffix_len = %i)\n",
    //	   (strand == 0 ? '+' : '-'), read_pos, num_suffixes, suffix_len);
    if (num_suffixes > 0) {
      for (size_t suff = low; suff <= high; suff++) {
	chrom = sa_index->CHROM[suff];
	if (chrom == chromosome) {
	  // extend suffix to right side
	  r_start_suf = read_pos;
	  r_end_suf = r_start_suf + suffix_len - 1;
	  
	  g_start_suf = sa_index->SA[suff] - sa_index->genome->chrom_offsets[chrom];
	  g_end_suf = g_start_suf + suffix_len - 1;
	  
	  //printf("region (chrom, start, end) = (%i, %lu, %lu), read_pos = %i, suffix = (%i, %lu, %lu)\n", 
	  //	 chromosome, start, end, read_pos, chrom, g_start_suf, g_end_suf);
	  
	  if (start <= g_start_suf && end >= g_end_suf) {
	    //printf("g_start_suf, g_end_suf) = (%lu, %lu)\n", g_start_suf, g_end_suf);
	    suffix_mng_update(chrom, r_start_suf, r_end_suf, g_start_suf, g_end_suf, suffix_mng);
	  }
	}
      }
    }
  }
  
  // using suffix manager instead of the previous cal manager
  suffix_mng_create_cals(read, read->length / 3, strand, 
			 sa_index, cal_list, suffix_mng);
}

//--------------------------------------------------------------------

void suffix_mng_display(suffix_mng_t *p) {
  if (p) {
    if (p->suffix_lists) {
      seed_t *seed;
      linked_list_t *suffix_list;
      for (unsigned int i = 0; i < p->num_chroms; i++) {
	suffix_list = p->suffix_lists[i];
	if (suffix_list) {
	  for (linked_list_item_t *item = suffix_list->first; 
	       item != NULL; 
	       item = item->next) {
	    seed = item->item;
	    print_seed("suffix mng display: ", seed);	    
	  }
	}
      }
    }
  }
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
