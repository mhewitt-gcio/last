// Copyright 2008, 2009, 2011, 2012, 2013, 2014 Martin C. Frith

#include "Alignment.hh"
#include "Alphabet.hh"
#include "GeneticCode.hh"
#include "TwoQualityScoreMatrix.hh"

#include <assert.h>

using namespace cbrc;

static void addSeedCounts(BigPtr seq1, const uchar *seq2, size_t size,
			  double *counts) {
  for (size_t i = 0; i < size; ++i) {
    ++counts[seq1[i] * scoreMatrixRowSize + seq2[i]];
  }
  counts[scoreMatrixRowSize * scoreMatrixRowSize] += size;
}

// Does x precede and touch y in both sequences?
static bool isNext( const SegmentPair& x, const SegmentPair& y ){
  return x.end1() == y.beg1() && x.end2() == y.beg2();
}

void Alignment::makeXdrop( Aligners &aligners, bool isGreedy, bool isFullScore,
			   BigSeq seq1, const uchar *seq2, int globality,
			   const ScoreMatrixRow* scoreMatrix,
			   int smMax, int smMin,
			   const const_dbl_ptr* probMatrix, double scale,
			   const GapCosts& gap, int maxDrop,
			   size_t frameSize, const ScoreMatrixRow* pssm2,
                           const TwoQualityScoreMatrix& sm2qual,
                           const uchar* qual1, const uchar* qual2,
			   const Alphabet& alph, AlignmentExtras& extras,
			   double gamma, int outputType ){
  std::vector<char> &columnAmbiguityCodes = extras.columnAmbiguityCodes;
  if (probMatrix) score = seed.score;  // else keep the old score
  if (outputType > 3 && !isFullScore) extras.fullScore = seed.score;
  blocks.clear();
  columnAmbiguityCodes.clear();

  if( outputType == 7 ){
    const int numOfTransitions = frameSize ? 9 : 5;
    std::vector<double> &ec = extras.expectedCounts;
    ec.assign(scoreMatrixRowSize * scoreMatrixRowSize + numOfTransitions, 0.0);
    addSeedCounts(seq1 + seed.beg1(), seq2 + seed.beg2(), seed.size, &ec[0]);
  }

  // extend a gapped alignment in the left/reverse direction from the seed:
  extend( aligners, isGreedy, isFullScore,
	  seq1, seq2, seed.beg1(), seed.beg2(), false, globality,
	  scoreMatrix, smMax, smMin, probMatrix, scale, maxDrop, gap,
	  frameSize, pssm2, sm2qual, qual1, qual2, alph,
	  extras, gamma, outputType );

  if( score == -INF ) return;  // maybe unnecessary?

  // convert left-extension coordinates to sequence coordinates:
  size_t seedBeg1 = seed.beg1();
  size_t seedBeg2 = aaToDna(seed.beg2(), frameSize);
  for (size_t i = 0; i < blocks.size(); ++i) {
    size_t s = blocks[i].size;
    blocks[i].start1 = seedBeg1 - blocks[i].start1 - s;
    // careful: start2 might be -1 (reverse frameshift)
    blocks[i].start2 = dnaToAa(seedBeg2 - blocks[i].start2, frameSize) - s;
  }

  bool isMergeSeedRev = !blocks.empty() && isNext(blocks.back(), seed);
  if (isMergeSeedRev) {
    blocks.back().size += seed.size;
  } else {
    blocks.push_back(seed);
  }

  if (outputType > 3) {  // set the un-ambiguity of the core to a max value:
    columnAmbiguityCodes.insert(columnAmbiguityCodes.end(), seed.size, 126);
  }

  size_t middle = blocks.size();
  size_t codesMid = columnAmbiguityCodes.size();

  // extend a gapped alignment in the right/forward direction from the seed:
  extend( aligners, isGreedy, isFullScore,
	  seq1, seq2, seed.end1(), seed.end2(), true, globality,
	  scoreMatrix, smMax, smMin, probMatrix, scale, maxDrop, gap,
	  frameSize, pssm2, sm2qual, qual1, qual2, alph,
	  extras, gamma, outputType );

  if( score == -INF ) return;  // maybe unnecessary?

  // convert right-extension coordinates to sequence coordinates:
  size_t seedEnd1 = seed.end1();
  size_t seedEnd2 = aaToDna(seed.end2(), frameSize);
  for (size_t i = middle; i < blocks.size(); ++i) {
    blocks[i].start1 = seedEnd1 + blocks[i].start1;
    // careful: start2 might be -1 (reverse frameshift)
    blocks[i].start2 = dnaToAa(seedEnd2 + blocks[i].start2, frameSize);
  }

  bool isMergeSeedFwd = blocks.size() > middle && isNext(seed, blocks.back());

  if (isMergeSeedFwd) {
    blocks[middle - 1].size += blocks.back().size;
    blocks.pop_back();
  }

  reverse(blocks.begin() + middle, blocks.end());
  reverse(columnAmbiguityCodes.begin() + codesMid, columnAmbiguityCodes.end());

  for (size_t i = middle; i < blocks.size(); ++i)
    blocks[i - 1].score = blocks[i].score;

  if (seed.size == 0 && !isMergeSeedRev && !isMergeSeedFwd) {
    // unusual, weird case: give up
    score = -INF;
    blocks[0].score = -1;
  }
}

// cost of the gap between x and y
static int gapCost(const SegmentPair &x, const SegmentPair &y,
		   const GapCosts &gapCosts, size_t frameSize) {
  if (gapCosts.isNewFrameshifts()) return x.score;
  size_t gapSize1 = y.beg1() - x.end1();
  size_t gapSize2, frameshift2;
  sizeAndFrameshift(x.end2(), y.beg2(), frameSize, gapSize2, frameshift2);
  int cost = gapCosts.cost(gapSize1, gapSize2);
  if (frameshift2) cost += gapCosts.frameshiftCost;
  return cost;
}

bool Alignment::isOptimal(BigSeq seq1, const uchar *seq2, int globality,
			  const ScoreMatrixRow *scoreMatrix, int maxDrop,
			  const GapCosts &gapCosts, size_t frameSize,
			  const ScoreMatrixRow *pssm2,
			  const TwoQualityScoreMatrix &sm2qual,
			  const uchar *qual1, const uchar *qual2) const {
  bool isLocal = !globality;
  size_t numOfBlocks = blocks.size();
  int maxScore = 0;
  int score = 0;

  for (size_t i = 0; i < numOfBlocks; ++i) {
    if (i > 0) {  // between each pair of aligned blocks:
      score -= gapCost(blocks[i - 1], blocks[i], gapCosts, frameSize);
      if ((isLocal && score <= 0) || score < maxScore - maxDrop) return false;
    }

    size_t x = blocks[i].beg1();
    size_t y = blocks[i].beg2();
    size_t blockLength = blocks[i].size;
    size_t theEnd = blockLength - (i+1 == numOfBlocks);

    if (sm2qual) {
      for (size_t j = 0; j < blockLength; ++j) {
	score += sm2qual(seq1[x+j], seq2[y+j], qual1[x+j], qual2[y+j]);
	if (score > maxScore) maxScore = score;
	else if ((isLocal && (score <= 0 || j == theEnd)) ||
		 score < maxScore - maxDrop) return false;
      }
    } else if (pssm2) {
      for (size_t j = 0; j < blockLength; ++j) {
	score += pssm2[y+j][seq1[x+j]];
	if (score > maxScore) maxScore = score;
	else if ((isLocal && (score <= 0 || j == theEnd)) ||
		 score < maxScore - maxDrop) return false;
      }
    } else {
      for (size_t j = 0; j < blockLength; ++j) {
	score += scoreMatrix[seq1[x+j]][seq2[y+j]];
	if (score > maxScore) maxScore = score;
	else if ((isLocal && (score <= 0 || j == theEnd)) ||
		 score < maxScore - maxDrop) return false;
      }
    }
  }

  return true;
}

bool Alignment::hasGoodSegment(BigSeq seq1, const uchar *seq2,
			       int minScore, const ScoreMatrixRow *scoreMatrix,
			       const GapCosts &gapCosts, size_t frameSize,
			       const ScoreMatrixRow *pssm2,
			       const TwoQualityScoreMatrix &sm2qual,
			       const uchar *qual1, const uchar *qual2) const {
  int score = 0;

  for (size_t i = 0; i < blocks.size(); ++i) {
    if (i > 0) {  // between each pair of aligned blocks:
      score -= gapCost(blocks[i - 1], blocks[i], gapCosts, frameSize);
      if (score < 0) score = 0;
    }

    size_t x = blocks[i].beg1();
    size_t y = blocks[i].beg2();
    size_t s = blocks[i].size;

    for (size_t j = 0; j < s; ++j) {
      score += sm2qual ? sm2qual(seq1[x+j], seq2[y+j], qual1[x+j], qual2[y+j])
	:      pssm2   ? pssm2[y+j][seq1[x+j]]
	:                scoreMatrix[seq1[x+j]][seq2[y+j]];

      if (score >= minScore) return true;
      if (score < 0) score = 0;
    }
  }

  return false;
}

static void getColumnCodes(const Centroid& centroid, std::vector<char>& codes,
			   const SegmentPair *chunks, size_t chunkCount,
			   bool isForward) {
  for (size_t i = 0; i < chunkCount; ++i) {
    const SegmentPair& x = chunks[i];
    centroid.getMatchAmbiguities(codes, x.end1(), x.end2(), x.size);
    size_t j = i + 1;
    bool isNext = (j < chunkCount);
    size_t end1 = isNext ? chunks[j].end1() : 0;
    size_t end2 = isNext ? chunks[j].end2() : 0;
    // ASSUMPTION: if there is an insertion adjacent to a deletion,
    // the deletion will get printed first.
    if (isForward) {
      centroid.getInsertAmbiguities(codes, x.beg2(), end2);
      centroid.getDeleteAmbiguities(codes, x.beg1(), end1);
    } else {
      centroid.getDeleteAmbiguities(codes, x.beg1(), end1);
      centroid.getInsertAmbiguities(codes, x.beg2(), end2);
    }
  }
}

static void getColumnCodes(const FrameshiftXdropAligner &fxa,
			   std::vector<char> &codes,
			   const SegmentPair *chunks, size_t chunkCount) {
  for (size_t i = 0; i < chunkCount; ++i) {
    const SegmentPair &x = chunks[i];
    for (size_t k = x.size; k-- > 0;) {
      double p = fxa.matchProb(x.beg1() + k, x.beg2() + k * 3);
      codes.push_back(asciiProbability(p));
    }
    size_t j = i + 1;
    bool isNext = (j < chunkCount);
    size_t end1 = isNext ? chunks[j].end1() : 0;
    size_t end2 = isNext ? chunks[j].beg2() + chunks[j].size * 3 : 0;
    size_t n1 = x.beg1() - end1;
    size_t n2 = (x.beg2() - end2 + 1) / 3;
    codes.insert(codes.end(), n1 + n2, '-');
  }
}

void Alignment::extend( Aligners &aligners, bool isGreedy, bool isFullScore,
			BigSeq seq1, const uchar* seq2,
			size_t start1, size_t start2,
			bool isForward, int globality,
			const ScoreMatrixRow* sm, int smMax, int smMin,
			const const_dbl_ptr* probMat, double scale,
			int maxDrop, const GapCosts& gap, size_t frameSize,
			const ScoreMatrixRow* pssm2,
			const TwoQualityScoreMatrix& sm2qual,
                        const uchar* qual1, const uchar* qual2,
			const Alphabet& alph, AlignmentExtras& extras,
			double gamma, int outputType ){
  const GapCosts::Piece &del = gap.delPieces[0];
  const GapCosts::Piece &ins = gap.insPieces[0];
  Centroid &centroid = aligners.centroid;
  GappedXdropAligner& aligner = centroid.aligner();
  GreedyXdropAligner &greedyAligner = aligners.greedyAligner;
  std::vector<char> &columnCodes = extras.columnAmbiguityCodes;
  size_t blocksBeg = blocks.size();

  double *subsCounts[scoreMatrixRowSize];
  double *tranCounts;
  if (outputType == 7) {
    double *ec = &extras.expectedCounts[0];
    for (int i = 0; i < scoreMatrixRowSize; ++i)
      subsCounts[i] = ec + i * scoreMatrixRowSize;
    tranCounts = ec + scoreMatrixRowSize * scoreMatrixRowSize;
  }

  if( frameSize ){
    assert( !isGreedy );
    assert( !globality );
    assert( !pssm2 );
    assert( !sm2qual );

    const uchar *s1 = seq1.beg + start1;
    const uchar *s2 = seq2 + start2;
    size_t dnaStart = aaToDna( start2, frameSize );
    size_t frame1 = isForward ? dnaStart + 1 : dnaStart - 1;
    const uchar *f1 = seq2 + dnaToAa(frame1, frameSize);
    size_t end1, end2, size;
    int gapCost;

    if (gap.isNewFrameshifts()) {
      assert(isFullScore);
      size_t frame2 = isForward ? dnaStart + 2 : dnaStart - 2;
      const uchar *f2 = seq2 + dnaToAa(frame2, frameSize);
      aligner.alignFrame(s1, s2, f1, f2, isForward, sm, gap, maxDrop);
      while (aligner.getNextChunkFrame(end1, end2, size, gapCost, gap))
	blocks.push_back(SegmentPair(end1 - size, end2 - size * 3, size,
				     gapCost));
      if (!probMat) return;
      FrameshiftXdropAligner &fxa = aligners.frameshiftAligner;
      double probDropLimit = exp(scale * -maxDrop);
      double s = fxa.forward(s1, s2, f1, f2,
			     isForward, probMat, gap, probDropLimit);
      score += s / scale;
      if (outputType < 4) return;
      fxa.backward(isForward, probMat, gap);
      getColumnCodes(fxa, columnCodes, blocks.data() + blocksBeg,
		     blocks.size() - blocksBeg);
      if (outputType == 7) fxa.count(isForward, gap, subsCounts, tranCounts);
    } else {
      assert(!isFullScore);
      assert(outputType < 4);
      size_t frame2 = isForward ? dnaStart - 1 : dnaStart + 1;
      const uchar *f2 = seq2 + dnaToAa(frame2, frameSize);
      score += aligner.align3(s1, s2, f1, f2, isForward,
			      sm, del.openCost, del.growCost, gap.pairCost,
			      gap.frameshiftCost, maxDrop, smMax);
      // This should be OK even if end2 < size * 3:
      while (aligner.getNextChunk3(end1, end2, size,
				   del.openCost, del.growCost, gap.pairCost,
				   gap.frameshiftCost))
	blocks.push_back(SegmentPair(end1 - size, end2 - size * 3, size));
    }

    return;
  }

  if (!isForward) {
    --start1;
    --start2;
  }
  const uchar *s2 = seq2 + start2;

  bool isSimdMatrix = (alph.size == 4 && !globality && gap.isAffine &&
		       smMin >= SCHAR_MIN &&
		       maxDrop + smMax * 2 - smMin < UCHAR_MAX);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      if (sm[i][j] != sm[alph.numbersToLowercase[i]][j])
	isSimdMatrix = false;

  int extensionScore =
    isGreedy  ? greedyAligner.align(seq1.beg + start1, s2,
				    isForward, sm, maxDrop, alph.size)
    : sm2qual ? aligner.align2qual(seq1.beg + start1, qual1 + start1,
				   s2, qual2 + start2,
				   isForward, globality, sm2qual,
				   del.openCost, del.growCost,
				   ins.openCost, ins.growCost,
				   gap.pairCost, gap.isAffine, maxDrop, smMax)
    : pssm2   ? aligner.alignPssm(seq1 + start1, pssm2 + start2,
				  isForward, globality,
				  del.openCost, del.growCost,
				  ins.openCost, ins.growCost,
				  gap.pairCost, gap.isAffine, maxDrop, smMax)
#if defined __SSE4_1__ || defined __ARM_NEON
    : isSimdMatrix ? aligner.alignDna(seq1 + start1, s2, isForward, sm,
				      del.openCost, del.growCost,
				      ins.openCost, ins.growCost,
				      maxDrop, smMax, alph.numbersToUppercase)
#endif
    :           aligner.align(seq1 + start1, s2, isForward, globality, sm,
			      del.openCost, del.growCost,
			      ins.openCost, ins.growCost,
			      gap.pairCost, gap.isAffine, maxDrop, smMax);

  if( extensionScore == -INF ){
    score = -INF;  // avoid score overflow
    return;  // avoid ill-defined probabilistic alignment
  }

  if( outputType < 5 || outputType > 6 ){  // ordinary max-score alignment
    size_t end1, end2, size;
    if( isGreedy ){
      while( greedyAligner.getNextChunk( end1, end2, size ) )
	blocks.push_back( SegmentPair( end1 - size, end2 - size, size ) );
    }
#if defined __SSE4_1__ || defined __ARM_NEON
    else if (isSimdMatrix && !pssm2 && !sm2qual) {
      while (aligner.getNextChunkDna(end1, end2, size,
				     del.openCost, del.growCost,
				     ins.openCost, ins.growCost))
	blocks.push_back(SegmentPair(end1 - size, end2 - size, size));
    }
#endif
    else {
      while( aligner.getNextChunk( end1, end2, size,
				   del.openCost, del.growCost,
				   ins.openCost, ins.growCost, gap.pairCost ) )
	blocks.push_back( SegmentPair( end1 - size, end2 - size, size ) );
    }
  }

  if (!probMat) return;
  if (!isFullScore) score += extensionScore;

  if (outputType > 3 || isFullScore) {
    assert( !isGreedy );
    assert( !sm2qual );
    double s = centroid.forward(seq1 + start1, s2, start2, isForward,
				probMat, gap, globality);
    if (isFullScore) {
      score += s / scale;
    } else {
      extras.fullScore += s / scale;
    }
    if (outputType < 4) return;
    centroid.backward(isForward, probMat, gap, globality);
    if (outputType > 4 && outputType < 7) {  // gamma-centroid / LAMA alignment
      centroid.dp(outputType, gamma);
      size_t beg1, beg2, length;
      while (centroid.traceback(beg1, beg2, length, outputType, gamma)) {
	blocks.push_back(SegmentPair(beg1, beg2, length));
      }
    }
    getColumnCodes(centroid, columnCodes, blocks.data() + blocksBeg,
		   blocks.size() - blocksBeg, isForward);
    if (outputType == 7) {
      centroid.addExpectedCounts(start2, isForward, probMat, gap, alph.size,
				 subsCounts, tranCounts);
    }
  }
}
