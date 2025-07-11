last-train
==========

last-train finds the rates (probabilities) of insertion, deletion, and
substitutions between two sets of sequences.  It thereby finds
suitable substitution and gap scores for aligning them.  You can use
it like this::

  lastdb mydb reference.fasta
  last-train mydb queries.fasta > my.train

last-train can read ``.gz`` files, or from pipes::

  bzcat queries.fasta.bz2 | last-train mydb > my.train

How it works
------------

1. For sake of speed, last-train just uses some pseudo-random chunks
   of ``queries.fasta``.

2. It starts with an initial guess for substitution and gap
   rates/scores.

3. Using these rates, it finds similar segments between the chunks and
   ``reference.fasta``.  (If one part of the chunks matches several
   parts of ``reference.fasta``, only the best matches are kept.)

4. It gets substitution and gap rates from these similar segments.

5. It uses these rates to find similar segments more accurately, then
   gets rates again, and repeats until the result stops changing.

last-train prints a summary of each iteration, followed by the final
score parameters in a format that can be read by `lastal's -p option
<doc/lastal.rst>`_.

Troubleshooting
---------------

If the query sequences rarely have segments similar to the reference
sequences, you may need to use more chunks (option
``--sample-number``), to include some such segments.

You can see how much training data was used at each iteration by
looking at ``aligned letter pairs`` and ``alignments`` in the output.

The ``--codon`` option needs much training data, to learn 64×21
substitution rates.  If there are few truly-related segments, it can
wrongly learn high substitution probabilities for unrelated sequences
next to those segments.  In the worst case, these unrelated sequences
are highly repeated.

Options
-------

-h, --help
       Show a help message, with default option values, and exit.
-v, --verbose
       Show more details of intermediate steps.

Training options
~~~~~~~~~~~~~~~~

--revsym
       Force the substitution scores to have reverse-complement
       symmetry, e.g. score(A→G) = score(T→C).  This is often
       appropriate, if neither strand is "special".
--matsym
       Force the substitution scores to have directional symmetry,
       e.g. score(A→G) = score(G→A).
--gapsym
       Force the insertion costs to equal the deletion costs.
--fixmat=NAME
       Fix the substitution probabilities to one of: ``BLOSUM45``,
       ``BLOSUM50``, ``BLOSUM62``, ``BLOSUM80``, ``BLOSUM90``,
       ``PAM30``, ``PAM70``, ``PAM250``.  (Note: this doesn't fix the
       substitution scores, which depend also on the probability of
       not having a gap.)
--pid=PID
       Ignore similar segments with > PID% identity (matches /
       [matches + mismatches]).  This aims to optimize the parameters
       for low-similarity alignments.
--postmask=NUMBER
       By default, last-train ignores alignments of mostly-lowercase
       sequence (by using `last-postmask <doc/last-postmask.rst>`_).
       To turn this off, do ``--postmask=0``.
--sample-number=N
       Use N randomly-chosen chunks of the query sequences.  The
       queries are chopped into fixed-length chunks (as if they were
       first concatenated into one long sequence).  If there are ≤ N
       chunks, all are picked.  Otherwise, if the final chunk is
       shorter, it is never picked.  0 means use everything.
--sample-length=L
       Use randomly-chosen chunks of length L.
--scale=S
       Output scores in units of 1/S bits.  Traditional values
       include 2 for half-bit scores and 3 for 1/3-bit scores.
       (Note that 1/3-bit scores essentially equal Phred scores
       a.k.a. decibans, because log10(2) ≈ 3/10.)  The default is to
       infer a scale from the initial score parameters.
--codon
       Do training for DNA query sequences versus protein reference
       sequences.  These options will be ignored: ``--revsym
       --matsym --gapsym --fixmat --postmask -q -p -S``.  If ``--pid``
       is used, matches are defined by the genetic code, which is
       inferred from the substitution rates.  When ``--codon`` is
       used, the "initial parameter options" are initial
       probabilities, not scores/costs.

All options below this point are passed to lastal to do the
alignments: they are described in more detail at `<doc/lastal.rst>`_.

Initial parameter options
~~~~~~~~~~~~~~~~~~~~~~~~~

-r SCORE   Initial match score.
-q COST    Initial mismatch cost.
-p NAME    Initial match/mismatch score matrix.
-a COST    Initial gap existence cost.
-b COST    Initial gap extension cost.
-A COST    Initial insertion existence cost.
-B COST    Initial insertion extension cost.
-F LIST    Initial frameshift probabilities (only used with ``--codon``).

Alignment options
~~~~~~~~~~~~~~~~~

-D LENGTH  Query letters per random alignment.  (See `here
           <doc/last-evalues.rst>`_.)
-E EG2     Maximum expected alignments per square giga.  (See `here
           <doc/last-evalues.rst>`_.)
-s NUMBER  Which query strand to use: 0=reverse, 1=forward, 2=both.
           If specified, this parameter is written in last-train's
           output, so it will override lastal's default.
-S NUMBER  Specify how to use the substitution score matrix for
           reverse strands.  If you use ``--revsym``, this makes no
           difference.  "0" means that the matrix is used as-is for
           all alignments.  "1" (the default) means that the matrix
           is used as-is for alignments of query sequence forward
           strands, and the complemented matrix is used for query
           sequence reverse strands.

           This parameter is always written in last-train's output,
           so it will override lastal's default.

-C COUNT   Before extending gapped alignments, discard any gapless
           alignment whose query range lies in COUNT other gapless
           alignments with higher score-per-length.  This aims to
           reduce run time.
-T NUMBER  Type of alignment: 0=local, 1=overlap.
-R DIGITS  Lowercase & simple-sequence options.  If specified, this is
           written in last-train's output, so it will override
           lastal's default.
-m COUNT   Maximum number of initial matches per query position.
-k STEP    Look for initial matches starting only at every STEP-th
           position in each query.
-P COUNT   Number of parallel threads.
-X NUMBER  How to score a match/mismatch involving N (for DNA) or X
           (otherwise).  By default, the lowest match/mismatch score
           is used. 0 means the default; 1 means treat reference
           Ns/Xs as fully-ambiguous letters; 2 means treat query
           Ns/Xs as ambiguous; 3 means treat reference and query
           Ns/Xs as ambiguous.

           If specified, this parameter is written in last-train's
           output, so it will override lastal's default.

-Q NAME    How to read the query sequences (the NAME is not
           case-sensitive)::

             Default         fasta
             "0", "fastx"    fasta or fastq: discard per-base quality data
             "1", "sanger"   fastq-sanger

           The ``fastq`` formats are described here:
           `<doc/lastal.rst>`_.  last-train assumes the per-base
           quality codes indicate substitution error probabilities,
           *not* insertion or deletion error probabilities.  If this
           assumption is dubious (e.g. for data with many insertion
           or deletion errors), it may be better to discard the
           quality data.  For ``fastq-sanger``, last-train finds the
           rates of substitutions not explained by the quality data
           (ideally, real substitutions as opposed to errors).

           If specified, this parameter is written in last-train's
           output, so it will override lastal's default.

Details
-------

last-train shows the gap probabilities at each iteration.  They
correspond to "Model A" in Figure 5A of btz576_:

=============  ========================
last-train     btz576_
=============  ========================
delOpenProb    α\ :sub:`D`
insOpenProb    α\ :sub:`I`
delExtendProb  β\ :sub:`D`
insExtendProb  β\ :sub:`I`
matchProb      γ
endProb        ω\ :sub:`D`, ω\ :sub:`I`
=============  ========================

* last-train gets most of the probabilities from the similar sequence
  segments that it finds.  But it gets these probabilities in a
  different way:

  - It assumes that ω\ :sub:`D` = ω\ :sub:`I`, and gets the unique
    value that satisfies "balanced length probability" (btz576_).

  - It gets φ\ :sub:`x` and ψ\ :sub:`y` by assuming "homogeneous
    letter probabilities" (btz576_).

* last-train converts between gap probabilities and gap scores as in
  Supplementary Section 3.1 of btz576_.

* last-train rounds the scores to integers, which makes them slightly
  inaccurate.  It then finds an adjusted scale factor (without
  changing the scores), which makes the integer-rounded scores
  correspond to homogeneous letter probabilities and balanced length
  probability.  It writes this adjusted scale (in nats, not bits) as a
  "-t" option for lastal, e.g. "-t4.4363".

* In rare cases, it may be impossible to find such an adjusted scale
  factor.  If that happens, last-train increases the original scale
  (to reduce the inaccuracy of integer rounding), until the problem
  goes away.

When ``--codon`` is used, the gap probabilities correspond to Figure 2
of DNA-versus-protein_:

=============  =========================
last-train     DNA-versus-protein_
=============  =========================
delOpenProb    α\ :sub:`D`
insOpenProb    α\ :sub:`I`
delExtendProb  β\ :sub:`D`
insExtendProb  β\ :sub:`I`
del-1          1 - δ\ :sub:`D`
del-2          1 - ε\ :sub:`D`
ins+1          1 - δ\ :sub:`I`
ins+2          1 - ε\ :sub:`I`
matchProb      Γ
endProb        ω\ :sub:`i`, ∛ω\ :sub:`D`
=============  =========================

It assumes that ω\ :sub:`D` = ω\ :sub:`i`\ :sup:`3`, and gets the
unique value that satisfies "balanced length probability"
(DNA-versus-protein_).

.. _btz576: https://doi.org/10.1093/bioinformatics/btz576
.. _DNA-versus-protein: https://doi.org/10.1109/TCBB.2022.3177855
.. _significance: doc/last-evalues.rst
