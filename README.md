HPG Aligner README

Welcome to HPG Aligner !

HPG Aligner is an ultrafast and highly sensitive Next-Generation Sequencing (NGS) read mapping.

COMPONENTS
----------

 hpg-aligner - The executable file to map DNA/RNA sequences


CONTACT
------- 
  You can contact any of the following developers:

    * Joaquín Tárraga (jtarraga@cipf.es)
    * Héctor Martínez (martineh@cipf.es)
    * Ignacio Medina (imedina@cipf.es)


DOWNLOAD and BUILDING
---------------------

  HPG Aligner has been opened to the community and released in GitHub, so you can download by invoking the following commands:

    $ git clone https://github.com/opencb/hpg-aligner.git
    Cloning into 'hpg-aligner'...
    remote: Reusing existing pack: 1441, done.
    remote: Total 1441 (delta 0), reused 0 (delta 0)
    Receiving objects: 100% (1441/1441), 1.85 MiB | 122 KiB/s, done.
    Resolving deltas: 100% (882/882), done.

    $ cd hpg-aligner

    $ git submodule update --init
    Submodule 'lib/hpg-libs' (https://github.com/opencb/hpg-libs.git) registered for path 'lib/hpg-libs'
    Cloning into 'lib/hpg-libs'...
    remote: Reusing existing pack: 7735, done.
    remote: Total 7735 (delta 0), reused 0 (delta 0)
    Receiving objects: 100% (7735/7735), 26.82 MiB | 79 KiB/s, done.
    Resolving deltas: 100% (4430/4430), done.
    Submodule path 'lib/hpg-libs': checked out '962f531ef0ffa2a6a665ae6fba8bc2337c4351a9'

  For the most recent HPG Aligner version, choose the 'master' Git branch (both for the hpg-libs and the HPG Aligner):

    $ cd lib/hpg-libs
    $ git checkout master
    $ cd ../..
    $ git checkout master


  Before you can build HPG Aligner, you must install on your system:

    * the GSL (GNU Scientific Library), http://www.gnu.org/software/gsl/
    * the Check library, http://check.sourceforge.net/

  Finally, use Scons to build the HPG Aligner application:

    $ scons

  For build HPG Aligner yo need Git, GCC, Scons, zlib, curl, xml, curses, gsl and check. If some of they are not install in your system, you can try for Ubuntu and Debian:
    

    $ apt-get install git 
    $ apt-get install gcc
    $ apt-get install scons
    $ apt-get install zlib1g-dev libcurl4-gnutls-dev libxml2-dev libncurses5-dev libgsl0-dev check

  And you can try for Centos and Fedora:
    
    $ yum install git 
    $ yum install gcc
    $ yum install scons
    $ yum install zlib-devel libcurl-devel libxml2-devel ncurses-devel gsl-devel check-devel


DOWNLOAD and BUILDING FOR MPI
-----------------------------

  HPG Aligner MPI (only available for RNA) has been opened to the community and released in GitHub, so you can download by invoking the following commands:

    $ git clone https://github.com/opencb/hpg-aligner.git
    Cloning into 'hpg-aligner'...
    remote: Reusing existing pack: 1441, done.
    remote: Total 1441 (delta 0), reused 0 (delta 0)
    Receiving objects: 100% (1441/1441), 1.85 MiB | 122 KiB/s, done.
    Resolving deltas: 100% (882/882), done.

    $ cd hpg-aligner

    $ git submodule update --init
    Submodule 'lib/hpg-libs' (https://github.com/opencb/hpg-libs.git) registered for path 'lib/hpg-libs'
    Cloning into 'lib/hpg-libs'...
    remote: Reusing existing pack: 7735, done.
    remote: Total 7735 (delta 0), reused 0 (delta 0)
    Receiving objects: 100% (7735/7735), 26.82 MiB | 79 KiB/s, done.
    Resolving deltas: 100% (4430/4430), done.
    Submodule path 'lib/hpg-libs': checked out '962f531ef0ffa2a6a665ae6fba8bc2337c4351a9'
  
  Choose the 'master' Git branch in hpg-libs and 'develop-mpi' in HPG Aligner:

    $ cd lib/hpg-libs
    $ git checkout master
    $ cd ../..
    $ git checkout develop-mpi

  Before you can build HPG Aligner MPI, you must install on your system:
    
    * the GSL (GNU Scientific Library), http://www.gnu.org/software/gsl/
    * the Check library, http://check.sourceforge.net/
    * the tcmalloc library, http://code.google.com/p/gperftools/
    * the mvapich2 or openMPI, http://mvapich.cse.ohio-state.edu/downloads/

  Finally, use Scons to build the HPG Aligner MPI application with mpicc compiler:

    $ scons compiler=mpicc

  
RUNING
-------

  For command line options invoke:

    $ ./bin/hpg-aligner -h


  In order to map DNA sequences:

    1) Create the HPG Aligner index based on suffix arrays by invoking:

      $ ./bin/hpg-aligner build-sa-index -g <ref-genome-fasta-file> -i <index-directory>

      Example:

      $./bin/hpg-aligner build-sa-index -g /hpg/genome/human/GRCH_37_ens73.fa -i /tmp/sa-index-human73/ 

    2) Map by invoking:

      $./bin/hpg-aligner dna -i <index-directory> -f <fastq-file> -o <output-directory>

      Example:

      $./bin/hpg-aligner dna -i /tmp/sa-index-human73/ -f /hpg/fastq/simulated/human/DNA/GRCh37_ens73/4M_100nt_r0.01.bwa.read1.fastq 
      ----------------------------------------------
      Loading SA tables...
      End of loading SA tables in 1.03 min. Done!!
      ----------------------------------------------
      Starting mapping...
      End of mapping in 1.40 min. Done!!
      ----------------------------------------------
      Output file        : hpg_aligner_output/out.sam
      Num. reads         : 4000000
      Num. mapped reads  : 3994693 (99.87 %)
      Num. unmapped reads: 5307 (0.13 %)
      ----------------------------------------------


  In order to map RNA sequences:

    A) Map Based on Burrows-Wheeler Transform method for slow memory consumption
       1) Create the BWT HPG Aligner index

         $ ./bin/hpg-aligner build-bwt-index -g <ref-genome-fasta-file> -i <index-directory> -r <compression-ratio>
	 
	 Example:

	 $ ./bin/hpg-aligner build-bwt-index -g /home/user/Homo_sapiens.fa -i /home/user/INDEX/  -r 8

       2) Map by invoking:

         $ ./bin/hpg-aligner rna -i <index-directory> -f <fastq-file> -o <output-directory>

         Example:

         $ ./bin/hpg-aligner rna -i /home/user/INDEX/ -f reads.fq
         +===============================================================+
         |                           RNA MODE                            |
         +===============================================================+
         |      ___  ___  ___                                            |
         |    \/ H \/ P \/ G \/                                          |
      	 |    /\___/\___/\___/\                                          |
         |      ___  ___  ___  ___  ___  ___  ___                        |
         |    \/ A \/ L \/ I \/ G \/ N \/ E \/ R \/                      |
         |    /\___/\___/\___/\___/\___/\___/\___/\                      |
         |                                                               |
         +===============================================================+

         WORKFLOW 1
         [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]  100%

         WORKFLOW 2
         [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]  100%

         WORKFLOW 3
         [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]  100%

         +===============================================================+
         |        H P G - A L I G N E R    S T A T I S T I C S           |
         +===============================================================+
         |              T I M E    S T A T I S T I C S                   |
         +---------------------------------------------------------------+
          Loading time                            :  5.69 (s)
          Alignment time                          :  438.09 (s)
          Total time                              :  443.79 (s)
         +---------------------------------------------------------------+
         |            M A P P I N G    S T A T I S T I C S               |
         +---------------------------------------------------------------+
          Total reads process                     :  10000000
          Total reads mapped                      :  9977505 (99.78%)
          Total reads unmapped                    :  22495 (0.22%)
          Reads with a single mapping             :  9532383 (95.32%)
          Reads with multi mappings               :  445122 (4.45%)
          Reads mapped with BWT phase             :  6441691 (64.42%)
          Reads mapped in workflow 1              :  9737937 (97.38%)
          Reads mapped in workflow 2              :  149381 (1.49%)
          Reads mapped in workflow 3              :  90187 (0.90%)
         +---------------------------------------------------------------+
         |    S P L I C E    J U N C T I O N S    S T A T I S T I C S    |
         +---------------------------------------------------------------+
          Total splice junctions                  :  2556653
          Total cannonical splice junctions       :  2535847 (99.19%)
          Total semi-cannonical splice junctions  :  20806 (0.81%)
         +---------------------------------------------------------------+

    B) Map Based on SA method for more speed

       1) Create the SA HPG Aligner index

         $ ./bin/hpg-aligner build-sa-index -g <ref-genome-fasta-file> -i <index-directory>
	 
	 Example:

	 $./bin/hpg-aligner build-sa-index  -g /home/user/Homo_sapiens.fa -i /home/user/INDEX/

       2) Map by invoking:

         $ ./bin/hpg-aligner rna -i <index-directory> -f <fastq-file> -o <output-directory> --sa-mode

         Example:

         $ ./bin/hpg-aligner rna  -i /home/user/INDEX/ -f reads.fq -o /home/user/sa_output --sa-mode
    

RUNING FOR RNA MPI
------------------
  
  For Map with MPI HPG Aligner for RNA, you can follow the same steps that HPG Aligner, but you should run the application with "mpirun" command. The number of process must be one more than the nodes that will be used (if you want use two nodes the number of process must be three) and the first and last process should be run in the same node:

        A) Example, run with MPI RNA and two nodes:

          $ mpirun -np 3 -hosts compute-0,compute-1,compute-0 ./bin/hpg-aligner rna -i index_path/ -f reads.fq 

        B) Example, run with MPI RNA and four nodes:

          $ mpirun -np 5 -hosts compute-0,compute-1,compute-2,compute-3,compute-0 ./bin/hpg-aligner rna -i index_path/ -f reads.fq 

  ATENTION: 
     Mvapich2 has an incompatibility with tcmalloc. You can install mvapich2 with "--disable-registration-cache" option or before you run HPG Aligner mpi do:

     	 $ export MV2_USE_LAZY_MEM_UNREGISTER=0


  If you compile MPI HPG Aligner without tcmalloc the perferomance will be degraded


DOCUMENTATION
-------------


  You can find more info about HPG Aligner project at:

  https://github.com/opencb/hpg-aligner/wiki
