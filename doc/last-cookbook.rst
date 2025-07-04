LAST Cookbook
=============

LAST_ is used by running commands in a terminal / command line.  It
has many options: unfortunately, the LAST developers don't know the
best options for every possible alignment task.  Here are some
reasonable starting points.  Feel free to optimize (and share!) search
protocols.

A minimal example: compare human & fugu mitochondrial genomes
-------------------------------------------------------------

Let's find and align similar regions between the human and fugu
mitochondrial genomes.  These FASTA-format files are in LAST's
examples directory: ``humanMito.fa`` and ``fuguMito.fa``.  The
simplest possible usage is::

  lastdb humdb humanMito.fa
  lastal humdb fuguMito.fa > myalns.maf

The lastdb_ command creates several files whose names begin with
"humdb".  The lastal_ command then compares fuguMito.fa to the humdb
files, and writes the alignments to a file called "myalns.maf".

Understanding the output
------------------------

The output has very long lines, so you need to view it without
line-wrapping.  For example, you can use::

  less -S myalns.maf

Each alignment looks like this (MAF_ format)::

  a score=39 EG2=0.092 E=4.9e-11
  s humanMito 12264 71 + 16571 CAACTTTTAAAGGATAACAGCTA-TCCATTGGTCTTAGGCCccaa...
  s fuguMito  11840 73 + 16447 CAGCTTTTGAAGGATAATAGCTAATCCGTTGGTCTTAGGAACCAA...


Score, EG2, and E are explained at last-evalues_.  Lines starting with
"s" contain: the sequence name, the start coordinate of the alignment,
the number of bases spanned by the alignment, the strand, the sequence
length, and the aligned bases.

The start coordinates are zero-based.  This means that, if the
alignment begins right at the start of a sequence, the coordinate is
0.  If the strand is "-", the start coordinate is the coordinate in
the reverse-complemented sequence (the same as if you were to
reverse-complement the sequence before giving it to LAST).

Bases judged (by tantan_) to be simple repeats are shown in lowercase,
e.g. ``cacacacacacacacacacacaca``.

You can convert MAF to other formats with maf-convert_, or use lastal_
option ``-f`` to get a few other formats.

More accurate: learn substitution & gap rates
---------------------------------------------

We can get more accurate alignments between the human and fugu
mitochondrial genomes like this::

  lastdb humdb humanMito.fa
  last-train humdb fuguMito.fa > hufu.train
  lastal -p hufu.train humdb fuguMito.fa > myalns.maf

The last-train_ command finds the rates of deletion, insertion, and
each kind of substitution between these sequences, and writes them to
a file "hufu.train".  lastal's ``-p`` option uses this file to get
more-accurate alignments.

Comparing protein sequences
---------------------------

We can compare some query proteins to some reference proteins like
this::

  lastdb -p -c refdb ref-prots.fa
  lastal refdb query-prots.fa > prot-alns.maf

``-p`` tells it the sequences are proteins.  (If you forget ``-p`` and
the sequences look proteinaceous, you'll get a warning message.)

``-c`` suppresses alignments caused by simple sequence such as
``appspappspappspappspap``.  It omits alignments that lack a
significant amount of uppercase-to-uppercase alignment.

You can also use last-train_, but we've hardly tested it for
protein-protein alignment, so we're not sure if it helps.

Find high-similarity, and short, protein alignments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If we just want high-similarity alignments, we can use the PAM30 (or
PAM10) `scoring scheme`_::

  lastdb -p -c refdb ref-prots.fa
  lastal -p PAM30 refdb query-prots.fa > prot-alns.maf

This has two advantages:

* It omits low-similarity alignments, or alignment parts.

* It can find short similarities, which would be deemed insignificant
  (likely to occur by chance between random sequences) unless we focus
  the search on high-similarity.

Comparing DNA to proteins
-------------------------

We can find related regions of DNA and proteins, allowing for nonsense
mutations and frameshifts.  For example, let's find DNA regions
related to transposon proteins::

  lastdb -q -c trandb transposon-prots.fa
  last-train --codon trandb dna.fa > codon.train
  lastal -p codon.train -m100 -D1e9 -K1 trandb dna.fa > out.maf

``-q`` appends ``*`` meaning STOP to each protein, and treats ``*`` as
a 21st protein letter.

``--codon`` makes it do DNA-versus-protein.  Here, last-train_ tries
to learn 21x64 substitution rates, so it needs a fairly large amount
of data (e.g. a chromosome).

``-m100`` makes it more slow-and-sensitive than the default (which is
``-m10``), see lastal_.

``-D1e9`` sets a strict significance_ threshold.  It means: only
report strong similarities that would be expected to occur by chance,
between random sequences, at most once per 10^9 base-pairs.  The
default is 1e6.

``-K1`` streamlines the output by omitting any alignment whose DNA
range lies in that of a higher-scoring alignment.

Another possibility is to add last-train_ option ``-X1``, which treats
matches to ``X`` (unknown) amino acids as neutral instead of
disfavored.

You can reuse ``last-train`` output for different alignment tasks, if
you expect the rates to be roughly the same.

Aligning high-indel-error long DNA reads to a genome
----------------------------------------------------

Suppose we have DNA reads in either FASTA or FASTQ_ format.  We can
align them to a genome like this::

  lastdb -P8 -uRY4 mydb genome.fa
  last-train -P8 -Q0 mydb reads.fastq > reads.train
  lastal -P8 --split -p reads.train mydb reads.fastq > out.maf

``-P8`` makes it faster by running 8 parallel threads, adjust as
appropriate for your computer.  This has no effect on the results.

``-uRY4`` selects a `seeding scheme`_ that reduces the run time and
memory use, but also reduces sensitivity.

``-Q0`` makes it discard the fastq_ quality information (or you can
keep-but-ignore it with ``-Qkeep``).

``--split`` cuts the output down to a unique best alignment for each
part of each read.  It gives each alignment a `mismap probability`_,
which is high if that part of the read is almost equally similar to
several parts of the genome.

Here we didn't suppress alignments caused by simple sequence (like
``cacacacacacacacacacacaca``), so as not to hide anything from
``--split``.  You can discard such alignments with last-postmask_
(though they may help to explain each part of a DNA read).

To make it more sensitive but slow, replace ``RY4`` with ``NEAR``:
good for smaller data.  (``-uNEAR`` is suitable for finding alignments
with few substitutions and/or many gaps.)

Aligning low-error long DNA reads to a genome
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We can do this the same way as for high-error reads, but perhaps
accelerate more aggressively.  ``RY8`` reduces the run time and memory
use even more than ``RY4``.  (This is because ``RY8`` uses ~1/8 of the
seeds, i.e. initial matches, whereas ``RY4`` uses ~1/4).  ``RY16``,
``RY32``, ``RY64``, and ``RY128`` are faster still.

Also, lastal_ option ``-C2`` may reduce run time with little effect on
accuracy.

Aligning potentially-spliced RNA or cDNA long reads to a genome
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See here_.  (For low-error reads, you can probably omit ``-d90`` and
``-m20``.)

Which genome version to use?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some genome versions (e.g. for human) have artificial
exactly-duplicated regions, which makes it hard to align reads
uniquely.  To avoid that, look for a genome version called something
like "analysis set".

You can use multiple genomes, which will be treated like one big
genome::

  lastdb -P8 -uRY4 mydb human.fa virus.fa other-genomes.fa

Aligning Illumina DNA reads to a genome
---------------------------------------

::

  lastdb -P8 -uNEAR mydb genome.fasta
  last-train -P8 -Q1 mydb reads.fastq.gz > reads.train
  lastal -P8 --split -p reads.train mydb reads.fastq.gz | gzip > out.maf.gz

Most LAST commands accept ``.gz`` compressed files, and you can
compress output with ``gzip`` as above.  You can get faster but
slightly worse compression with e.g. ``gzip -5``.

``-Q1`` makes it use the fastq_ quality information to improve the
training and alignment.  LAST **assumes** that the qualities reflect
substitution errors, not insertion/deletion errors.  (For long
non-Illumina reads, we suspect this assumption doesn't hold, so we
didn't use this option.)

This recipe may be excessively slow-and-sensitive.  Adding lastal_
option ``-C2`` may make it faster with negligible accuracy loss.  You
can accelerate with e.g. ``-uRY16`` as above.

Finding very short DNA alignments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, LAST only reports significant_ alignments that will rarely
occur by chance.  In the preceding example, the minimum alignment
length is about 26 bases for a human-size genome (less for smaller
genomes).  To find shorter alignments, add lastal_ option ``-D100``
(say), to get alignments that could occur by chance once per hundred
query letters (the default is once per million.)  This makes the
minimum alignment length about 20 bases for a human-size genome.

Aligning paired DNA reads to a genome
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Suppose we have paired reads in 2 files, ``reads1.fastq`` and
``reads2.fastq``.  The simplest thing is::

   lastal -P8 -2 --split -p reads.train mydb reads1.fastq reads2.fastq > out.maf

``-2`` organizes the results for paired reads next to each other.  It
doesn't use the pairing to inform the alignments: to do that, try
last-split-pe_ (or the older last-pair-probs_).

Aligning potentially-spliced Illumina reads to a genome
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See last-split_ (and last-pair-probs_).

Aligning human & chimp genomes
------------------------------

The aim of genome-genome alignment is discussed in `our paper`_.  This
recipe gets one-to-one alignments::

  lastdb -P8 -c -U400 -uRY128 humdb human.fa
  last-train -P8 --revsym -C2 humdb chimp.fa > hc.train
  lastal -P8 -D1e9 -C2 --split-f=MAF+ -p hc.train humdb chimp.fa | last-split -r | maf-linked - > out.maf

``-c -U400`` suppresses tandem repeats with repeat-unit length ≤ 400.
(The default is 100).  This prevents ``lastal`` using huge amounts of
memory and time on many ways of aligning centromeric repeats.

``-uRY128`` makes it faster but less sensitive: it will miss tiny
rearranged fragments.  To find such fragments, try ``-uRY4``.

``lastal`` may need too much memory, especially if you don't use ``RY128``.
You can make it use less memory by using fewer threads (``-P``).

``--revsym`` makes the substitution rates the same on both strands.
For example, it makes A→G equal T→C (because A→G on one strand means
T→C on the other strand).  This is usually appropriate for
genome-genome comparison (but maybe not for mitochondria with
asymmetric "heavy" and "light" strands).

``--split-f=MAF+`` has the same effect as ``--split`` (get a unique
best alignment for each part of chimp), and gets `per-base mismap
probabilities`_: the probability that each chimp base should be
aligned to a different part of human.

``last-split -r`` cuts these alignments down to a unique best
alignment for each part of human.  It uses the *per-base* mismap
probabilities to get the final *per-alignment* mismap probabilities.
(If you won't use the mismap probabilities, you can replace
``--split-f=MAF+`` with ``--split``.)

maf-linked_ removes isolated alignments, that are not "linked" to
other alignments (i.e. nearby in both genomes).  This removes
alignments between non-homologous insertions of homologous transposons
(Frith2022_).

Finally, we can make a dotplot_ (out.png)::

  last-dotplot out.maf

Aligning more distantly related genomes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Human versus old-world monkey (~7% base-pair substitutions): the
human-chimp recipe without ``-U400`` seems good.  (But if you use
``-uRY4``, ``-U400`` may reduce the memory usage.)

Human versus new-world monkey (~12% base-pair substitutions):
``RY128`` misses ~10% of related base-pairs.  ``RY4`` is more
sensitive.  No need for ``-U400``.

Human versus prosimian (~20% base-pair substitutions): ``RY4`` misses
~15% of related base-pairs.  For finer sensitivity, run ``lastdb``
with default ``-u``::

  lastdb -P8 -c humdb human.fa
  last-train -P8 --revsym -C2 humdb lemur.fa > hl.train
  lastal -P8 -m2 -D1e9 -C2 --split-f=MAF+ -p hl.train humdb lemur.fa | last-split -r | maf-linked - > out.maf

``-m2`` halves the time and memory use, and slightly reduces sensitivity.

This recipe is okay for more distantly-related genomes too.  For
higher sensitivity, omit ``-m2``.  To make it even more sensitive (but
slow and memory-consuming), add ``lastdb`` option ``-uMAM4``.  Yet
more slow-and-sensitive is ``-uMAM8``.

For whole genome dotplots, high sensitivity is not necessary.
``RY128`` is fine for e.g. human versus crocodile, though not for
human versus fish.

Moar faster
-----------

* `Using multiple CPUs / cores <doc/last-parallel.rst>`_
* `Various speed & memory options <doc/last-tuning.rst>`_

Ambiguity of alignment columns
------------------------------

Consider this alignment::

  TGAAGTTAAAGGTATATGAATTCCAATTCTTAACCCCCCTATTAAACGAATATCTTG
  |||||||| ||||||  |  ||  | |  |    || ||||||   |||||||||||
  TGAAGTTAGAGGTAT--GGTTTTGAGTAGT----CCTCCTATTTTTCGAATATCTTG

The middle section has such weak similarity that its precise alignment
cannot be confidently inferred.  We can see the confidence of each
alignment column with lastal_ option ``-j4``::

  lastal -j4 -p hufu.train humdb fuguMito.fa > myalns.maf

The output looks like this::

  a score=17 EG2=9.3e+09 E=5e-06
  s seqX 0 57 + 57 TGAAGTTAAAGGTATATGAATTCCAATTCTTAACCCCCCTATTAAACGAATATCTTG
  s seqY 0 51 + 51 TGAAGTTAGAGGTAT--GGTTTTGAGTAGT----CCTCCTATTTTTCGAATATCTTG
  p                %*.14442011.(%##"%$$$$###""!!!""""&'(*,340.,,.~~~~~~~~~~~

The "p" line indicates the probability that each column is wrongly
aligned, using a compact code (based on ASCII_, the same as fastq_
format):

======  =================   ======  =================
Symbol  Error probability   Symbol  Error probability
======  =================   ======  =================
``!``   0.79 -- 1           ``0``   0.025 -- 0.032
``"``   0.63 -- 0.79        ``1``   0.02  -- 0.025
``#``   0.5  -- 0.63        ``2``   0.016 -- 0.02
``$``   0.4  -- 0.5         ``3``   0.013 -- 0.016
``%``   0.32 -- 0.4         ``4``   0.01  -- 0.013
``&``   0.25 -- 0.32        ``5``   0.0079 -- 0.01
``'``   0.2  -- 0.25        ``6``   0.0063 -- 0.0079
``(``   0.16 -- 0.2         ``7``   0.005  -- 0.0063
``)``   0.13 -- 0.16        ``8``   0.004  -- 0.005
``*``   0.1  -- 0.13        ``9``   0.0032 -- 0.004
``+``   0.079 -- 0.1        ``:``   0.0025 -- 0.0032
``,``   0.063 -- 0.079      ``;``   0.002  -- 0.0025
``-``   0.05  -- 0.063      ``<``   0.0016 -- 0.002
``.``   0.04  -- 0.05       ``=``   0.0013 -- 0.0016
``/``   0.032 -- 0.04       ``>``   0.001  -- 0.0013
======  =================   ======  =================

Note that each alignment is grown from a "core" region, and the
ambiguity estimates assume that the core is correctly aligned.  The
core is indicated by "~" symbols, and it contains exact matches only.

.. _last: README.rst
.. _lastdb: doc/lastdb.rst
.. _lastal: doc/lastal.rst
.. _dotplot: doc/last-dotplot.rst
.. _last-pair-probs: doc/last-pair-probs.rst
.. _last-postmask: doc/last-postmask.rst
.. _per-base mismap probabilities:
.. _mismap probability:
.. _last-split: doc/last-split.rst
.. _last-train: doc/last-train.rst
.. _maf-convert: doc/maf-convert.rst
.. _maf-linked: doc/maf-linked.rst
.. _scoring scheme: doc/last-matrices.rst
.. _seeding scheme:
.. _seeding: doc/last-seeds.rst
.. _last-evalues:
.. _significant:
.. _significance: doc/last-evalues.rst
.. _tantan: https://gitlab.com/mcfrith/tantan
.. _last-split-pe: https://bitbucket.org/splitpairedend/last-split-pe/wiki/Home
.. _fastq: https://doi.org/10.1093/nar/gkp1137
.. _here:
.. _mask repeats: https://github.com/mcfrith/last-rna/blob/master/last-long-reads.md
.. _MAF: http://genome.ucsc.edu/FAQ/FAQformat.html#format5
.. _ASCII: https://en.wikipedia.org/wiki/ASCII
.. _Frith2022: https://doi.org/10.1093/molbev/msac068
.. _our paper: https://doi.org/10.1186/s13059-015-0670-9
