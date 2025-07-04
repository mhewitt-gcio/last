Detailed papers on LAST
=======================

If you'd like to cite LAST, choose the most relevant paper(s).  If in
doubt, the first one is ok.

1. `Adaptive seeds tame genomic sequence comparison`__.  Kiełbasa SM,
   Wan R, Sato K, Horton P, Frith MC.  Genome Res. 2011 21(3):487-93.

   __ http://genome.cshlp.org/content/21/3/487.long

   This describes the main algorithms used by LAST.

2. `Incorporating sequence quality data into alignment improves DNA
   read mapping`__.  Frith MC, Wan R, Horton P.  Nucleic Acids
   Res. 2010 38(7):e100.

   __ http://nar.oxfordjournals.org/content/38/7/e100.long

   How LAST uses sequence quality data.

3. `Parameters for Accurate Genome Alignment`__.  Frith MC, Hamada M,
   Horton P.  BMC Bioinformatics. 2010 11:80.

   __ http://www.biomedcentral.com/1471-2105/11/80

   Choice of score parameters, ambiguity of alignment columns, and
   gamma-centroid alignment.

4. `A new repeat-masking method enables specific detection of
   homologous sequences`__.  Frith MC.  Nucleic Acids Res. 2011
   39(4):e23.

   __ http://nar.oxfordjournals.org/content/39/4/e23.long

   This describes the tantan algorithm for finding simple /
   low-complexity / tandem repeats, which reliably prevents
   non-homologous alignments, unlike other repeat finders.

5. `Gentle masking of low-complexity sequences improves homology
   search`__.  Frith MC.  PLoS One. 2011 6(12):e28819.

   __ http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0028819

   This describes what LAST does with repeats after they have been
   found.

6. `Probabilistic alignments with quality scores: an application to
   short-read mapping toward accurate SNP/indel detection`__.  Hamada
   M, Wijaya E, Frith MC, Asai K.  Bioinformatics. 2011
   27(22):3085-92.

   __ http://bioinformatics.oxfordjournals.org/content/27/22/3085.long

   Describes probabilistic alignment using sequence quality data, and
   LAMA alignment.

7. `A mostly traditional approach improves alignment of
   bisulfite-converted DNA`__.  Frith MC, Mori R, Asai K.  Nucleic
   Acids Res. 2012 40(13):e100.

   __ http://nar.oxfordjournals.org/content/40/13/e100.long

   This describes alignment of bisulfite-converted DNA, and an update
   for use of fastq quality data that allows for non-uniform base
   frequencies.

8. `An approximate Bayesian approach for mapping paired-end DNA reads
   to a reference genome`__.  Shrestha AM, Frith MC.
   Bioinformatics. 2013 29(8):965-72.

   __ http://bioinformatics.oxfordjournals.org/content/29/8/965.long

   This describes the algorithm used by ``last-pair-probs``.

9. `Improved search heuristics find 20,000 new alignments between
   human and mouse genomes`__.  Frith MC, Noé L.  Nucleic Acids
   Res. 2014 42(7):e59.

   __ http://nar.oxfordjournals.org/content/42/7/e59.long

   This describes sensitive DNA seeding (MAM8 and MAM4).

10. `Frameshift alignment: statistics and post-genomic
    applications`__.  Sheetlin SL, Park Y, Frith MC, Spouge JL.
    Bioinformatics. 2014 30(24):3575-82.

    __ http://bioinformatics.oxfordjournals.org/content/30/24/3575.long

    Describes "old-style" DNA-versus-protein alignment allowing for
    frameshifts (without ``last-train``).

11. `Split-alignment of genomes finds orthologies more accurately`__.
    Frith MC, Kawaguchi R.  Genome Biology. 2015 16:106.

    __ http://www.genomebiology.com/content/16/1/106

    Describes the split alignment algorithm, and its application to
    whole genome alignment.

12. `Training alignment parameters for arbitrary sequencers with
    LAST-TRAIN`__.  Hamada M, Ono Y, Asai K Frith MC.
    Bioinformatics. 2017 33(6):926-928.

    __ https://academic.oup.com/bioinformatics/article-lookup/doi/10.1093/bioinformatics/btw742

    Describes ``last-train``.

13. `A Simplified Description of Child Tables for Sequence Similarity
    Search`__.  Frith MC, Shrestha A.  IEEE/ACM Trans Comput Biol
    Bioinform. 2018.

    __ https://ieeexplore.ieee.org/document/8288582/

    Describes how LAST uses child tables.

14. `Minimally-overlapping words for sequence similarity search`__.
    Frith MC, Noé L, Kucherov G.  Bioinformatics. 2020

    __ https://doi.org/10.1093/bioinformatics/btaa1054

    Describes the ``lastdb -u RY`` sparsity options, for LAST version < 1407.

15. `Improved DNA-versus-protein homology search for protein fossils`__.
    Yao Y, Frith MC.  IEEE/ACM Trans Comput Biol Bioinform. 2022

    __ https://doi.org/10.1109/TCBB.2022.3177855

    Describes "new-style" DNA-versus-protein search with
    ``last-train --codon``.

16. `How to optimally sample a sequence for rapid analysis`__.
    Frith MC, Shaw J, Spouge JL.

    __ https://doi.org/10.1101/2022.08.18.504476

    Describes the ``lastdb -u RY`` sparsity options, for LAST version >= 1407.

17. `A simple theory for finding related sequences by adding
    probabilities of alternative alignments`__.  Frith MC

    __ https://doi.org/10.1101/2023.09.26.559458

    Describes the ``lastal -J1`` option for sensitive detection of
    related sequence regions, considering alternative ways of aligning
    the regions.

External methods
----------------

LAST of course owes its ideas to much previous research.  Here are
listed only implementations that are directly used in LAST.

* `The Gumbel pre-factor k for gapped local alignment can be estimated
  from simulations of global alignment`__.  Sheetlin S, Park Y, Spouge
  JL.  Nucleic Acids Res. 2005 33(15):4987-94.

  __ http://nar.oxfordjournals.org/content/33/15/4987.long

  Describes how E-values are calculated.

* `New finite-size correction for local alignment score
  distributions`__.  Park Y, Sheetlin S, Ma N, Madden TL, Spouge JL.
  BMC Res Notes. 2012 5:286.

  __ http://www.biomedcentral.com/1756-0500/5/286

  Describes a correction that makes the E-values more accurate for
  short sequences.
