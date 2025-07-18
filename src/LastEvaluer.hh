// Copyright 2015 Martin C. Frith

// This class calculates E-values for pair-wise local alignment.
// It has 2 states: "good" and "bad".  It starts in the "bad" state.

// "database" = sequence1, "query" = sequence2

// For DNA-versus-protein alignment:
// protein = "database" = sequence1, DNA = "query" = sequence2

// "deletion" means deletion in sequence 2 relative to sequence 1
// "insertion" means insertion in sequence 2 relative to sequence 1

// A length-k deletion costs delOpen + k * delEpen
// A length-k insertion costs insOpen + k * insEpen

#ifndef LAST_EVALUER_HH
#define LAST_EVALUER_HH

#include "ScoreMatrixRow.hh"
#include "mcf_frameshift_xdrop_aligner.hh"

#include "alp/sls_alignment_evaluer.hpp"

namespace cbrc {

using namespace mcf;

class GeneticCode;

class LastEvaluer {
public:
  // This routine tries to initialize the object for a given set of
  // alignment parameters.  It may fail, i.e. set the object to the
  // "bad" state and throw an Sls::error.
  // These arguments are only used to lookup pre-calculated cases:
  // matrixName, matchScore, mismatchCost, geneticCodeName.
  // DNA-versus-protein alignment is indicated by: frameshiftCost >= 0.
  // As a special case, frameshiftCost==0 means no frameshifts.
  // For DNA-versus-protein alignment, letterFreqs2 should either be
  // NULL or point to 64 codon frequencies (aaa, aac, etc).
  void init(const char *matrixName,
	    int matchScore,
	    int mismatchCost,
	    const char *alphabet,
	    const ScoreMatrixRow *scoreMatrix,  // score[sequence1][sequence2]
	    const double *letterFreqs1,
	    const double *letterFreqs2,
	    bool isGapped,
	    int delOpen,
	    int delEpen,
	    int insOpen,
	    int insEpen,
	    int frameshiftCost,
	    const GeneticCode &geneticCode,
	    const char *geneticCodeName,
	    int verbosity);

  // initFullScores sets up for sum-of-paths local alignment scores.
  // isFrameshift=true implements section 2.6 of "Improved
  // DNA-versus-protein homology search for protein fossils", Y Yao &
  // MC Frith.  (It doesn't check whether Equation 3 is satisfied.)
  // isFrameshift=false does the equivalent for "model A" in "How sequence
  // alignment scores correspond to probability models", MC Frith
  // 2020, Bioinformatics 36(2):408-415.

  // The Freqs need not sum to 1.
  // substitutionProbs is S' in [Yao & Frith 2021].
  // scale  =  lambda  =  1/t in [Yao & Frith 2021].

  void initFullScores(const const_dbl_ptr *substitutionProbs,
		      const double *letterFreqs1, int alphabetSize1,
		      const double *letterFreqs2, int alphabetSize2,
		      const GapCosts &gapCosts, double scale,
		      int numOfAlignments, int seqLength, int verbosity,
		      bool isFrameshift = false);

  void setSearchSpace(double databaseTotSeqLength,
		      double databaseMaxSeqLength,
		      double queryTotSeqLength,
		      double queryMaxSeqLength,
		      double numOfStrands) {  // 1 or 2
    if (databaseMaxSeqLength > 0) {
      databaseMaxSeqLen = databaseMaxSeqLength;
      areaMultiplier = databaseTotSeqLength / databaseMaxSeqLen * numOfStrands;
    } else {
      databaseMaxSeqLen = 1;  // ALP doesn't like 0
      areaMultiplier = 0;
    }
    databaseLenMultiplier = areaMultiplier;
    queryMaxSeqLen = queryMaxSeqLength;
    if (queryMaxSeqLength > 0) {
      areaMultiplier *= queryTotSeqLength / queryMaxSeqLen;
    }
  }

  bool isGood() const { return evaluer.isGood(); }

  // Don't call this in the "bad" state:
  double evaluePerArea(double score) const
  { return evaluer.evaluePerArea(score); }

  // Don't call this in the "bad" state or before calling setSearchSpace:
  double area(double score, double queryLength) const {
    double q = (queryMaxSeqLen > 0) ? queryMaxSeqLen : queryLength;
    return areaMultiplier * evaluer.area(score, q, databaseMaxSeqLen);
  }

  // Don't call this in the "bad" state:
  double bitScore(double score) const { return evaluer.bitScore(score); }

  // Returns max(0, score with E-value == "evalue").
  // Don't call this in the "bad" state.
  double minScore(double evalue, double area) const;
  double minScore(double evalue, double seqLength1, double seqLength2) const;

  // Returns max(0, score with E-value == 1 per this many query letters).
  // Don't call this in the "bad" state or before calling setSearchSpace.
  double minScore(double queryLettersPerRandomAlignment) const {
    if (databaseLenMultiplier <= 0) return 0;
    double qryLen = 1e9;
    double evalue = qryLen / queryLettersPerRandomAlignment;
    return minScore(evalue / databaseLenMultiplier, databaseMaxSeqLen, qryLen);
  }

  // Returns max(0, score with all-sequences E-value == "evalue").
  // Don't call this in the "bad" state or before calling setSearchSpace.
  double allSeqsMinScore(double evalue) const {
    if (areaMultiplier <= 0) return 0;
    return minScore(evalue/areaMultiplier, databaseMaxSeqLen, queryMaxSeqLen);
  }

  // Writes some parameters preceded by "#".  Does nothing in the "bad" state.
  void writeCommented(std::ostream& out) const;

  // Writes all parameters in full precision.  Does nothing in the "bad" state.
  void writeParameters(std::ostream& out) const;

private:
  Sls::AlignmentEvaluer evaluer;
  double databaseMaxSeqLen;
  double databaseLenMultiplier;
  double queryMaxSeqLen;
  double areaMultiplier;
};

}

#endif
