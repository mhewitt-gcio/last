// Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014 Martin C. Frith

// This struct holds the command line arguments for lastal.

#ifndef LASTAL_ARGUMENTS_HH
#define LASTAL_ARGUMENTS_HH

#include "SequenceFormat.hh"
#include "split/last_split_options.hh"

#include <limits.h>
#include <stddef.h>

#include <iosfwd>
#include <string>
#include <vector>

namespace cbrc{

struct LastalArguments{
  // set the parameters to their default values:
  LastalArguments();

  // set parameters from a list of arguments:
  void fromArgs( int argc, char** argv, bool optionsOnly = false );

  // set parameters from a command line (by splitting it into arguments):
  void fromLine( const std::string& line );

  // set parameters from lines beginning with "#last":
  void fromString( const std::string& s );

  void resetCumulativeOptions() { verbosity = 0; }

  // set default option values that depend on input files:
  void setDefaultsFromAlphabet(bool isDna, bool isProtein, int refStrand,
			       bool isKeepRefLowercase, int refTantanSetting,
			       bool isCaseSensitiveSeeds, bool isVolumes,
			       size_t refMinimizerWindow);
  void setDefaultsFromMatrix(double lambda, double minScore,
			     double perSquareGigaDefault);

  // write the parameter settings, starting each line with "#":
  void writeCommented( std::ostream& stream ) const;

  // are we doing translated alignment (DNA versus protein)?
  bool isTranslated() const { return !frameshiftCosts.empty(); }

  // are we doing translated alignment with frameshifts?
  bool isFrameshift() const {
    return
      isTranslated() && (frameshiftCosts.size() > 1 || frameshiftCosts[0] > 0);
  }

  // get the name of the substitution score matrix:
  const char *matrixName(bool isDna, bool isProtein) const {
    if (matrixFile.empty() && matchScore < 0 && mismatchCost < 0
	&& !isGreedy) {
      if (isDna) return "HUMSUM";
      if (isProtein) return isTranslated() ? "BL80" : "BL62";
    }
    return matrixFile.c_str();
  }

  bool isSumOfPaths() const {
    return outputType > 3 || (scoreType != 0 && outputType > 1);
  }

  // how many strands are we scanning (1 or 2)?
  int numOfStrands() const{ return (strand == 2) ? 2 : 1; }

  int minGapCost(int gapLength) const {
    int m = INT_MAX;
    for (size_t i = 0; i < delOpenCosts.size(); ++i) {
      m = std::min(m, delOpenCosts[i] + gapLength * delGrowCosts[i]);
    }
    for (size_t i = 0; i < insOpenCosts.size(); ++i) {
      m = std::min(m, insOpenCosts[i] + gapLength * insGrowCosts[i]);
    }
    return m;
  }

  // options:
  int outputFormat;
  int outputType;
  int scoreType;
  int strand;
  bool isReverseQuerySequences;
  bool isQueryStrandMatrix;
  bool isGreedy;
  int globality;  // type of alignment: local, semi-global, etc.
  bool isPairedQuerySequences;
  bool isKeepLowercase;
  int tantanSetting;
  int maxRepeatUnit;
  int maskLowercase;
  double expectedAlignments;
  double expectedPerSquareGiga;
  double queryLettersPerRandomAlignment;
  double minScoreGapped;
  int minScoreGapless;
  int matchScore;
  int mismatchCost;
  std::vector<int> delOpenCosts;
  std::vector<int> delGrowCosts;
  std::vector<int> insOpenCosts;
  std::vector<int> insGrowCosts;
  int gapPairCost;
  std::vector<int> frameshiftCosts;
  std::string matrixFile;
  int ambiguousLetterOpt;
  int maxDropGapped;
  char maxDropGappedSuffix;
  int maxDropGapless;
  int maxDropFinal;
  char maxDropFinalSuffix;
  sequenceFormat::Enum inputFormat;
  size_t minHitDepth;
  size_t maxHitDepth;
  size_t oneHitMultiplicity;
  size_t maxGaplessAlignmentsPerQueryPosition;
  size_t maxAlignmentsPerQueryStrand;
  size_t cullingLimitForGaplessAlignments;
  size_t cullingLimitForFinalAlignments;
  size_t queryStep;
  size_t minimizerWindow;
  size_t batchSize;  // approx size of query sequences to scan in 1 batch
  unsigned numOfThreads;
  size_t maxRepeatDistance;  // suppress repeats <= this distance apart
  double temperature;  // probability = exp( score / temperature ) / Z
  double gamma;        // parameter for gamma-centroid alignment
  std::string geneticCodeFile;
  int verbosity;

  int gumbelSimSequenceLength;
  int gumbelSimAlignmentCount;

  bool isSplit;
  LastSplitOptions splitOpts;

  // positional arguments:
  const char* programName;
  std::string lastdbName;
  int inputStart;  // index in argv of first input filename
};

}

#endif
