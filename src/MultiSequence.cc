// Copyright 2008, 2009, 2010, 2011 Martin C. Frith

#include "MultiSequence.hh"
#include "io.hh"
#include <sstream>
#include <cassert>
#include <streambuf>

namespace cbrc {

void MultiSequence::initForAppending(size_t padSizeIn,
				     bool isAppendStopSymbol) {
  padSize = padSizeIn;
  seq.v.assign( padSize, ' ' );
  ends.v.assign( 1, padSize );
  names.v.clear();
  nameEnds.v.assign( 1, 0 );
  qualityScoresPerLetter = 0;
  isAppendingStopSymbol = isAppendStopSymbol;
}

void MultiSequence::reinitForAppending(){
  size_t n = finishedSequences();
  size_t s = padBeg(n);

  seq.v.erase(seq.v.begin(), seq.v.begin() + s);
  names.v.erase(names.v.begin(), names.v.begin() + nameEnds.v[n]);
  ends.v.resize(1);
  nameEnds.v.resize(1);
  if( !names.v.empty() ) nameEnds.v.push_back( names.v.size() );

  qualityScores.v.erase(qualityScores.v.begin(),
			qualityScores.v.begin() + s * qualsPerLetter());

  if (!pssm.empty()) {
    pssm.erase(pssm.begin(), pssm.begin() + s * scoreMatrixRowSize);
  }
}

void MultiSequence::fromFiles(const std::string &baseName, size_t seqCount,
			      size_t qualitiesPerLetter, bool is4bit,
			      bool isSmallCoords) {
  if (isSmallCoords) {
    ends4.m.open(baseName + ".ssp", seqCount + 1);
    nameEnds4.m.open(baseName + ".sds", seqCount + 1);
  } else {
    ends.m.open(baseName + ".ssp", seqCount + 1);
    nameEnds.m.open(baseName + ".sds", seqCount + 1);
  }

  size_t seqLength = getEnd(seqCount);
  seq.m.open(baseName + ".tis", (seqLength + is4bit) / (is4bit + 1));
  theSeqPtr.beg = seq.m.begin();
  theSeqPtr.is4bit = is4bit;
  names.m.open(baseName + ".des", getNameEnd(seqCount));
  padSize = getEnd(0);

  qualityScores.m.open(baseName + ".qua", seqLength * qualitiesPerLetter);
  qualityScoresPerLetter = qualitiesPerLetter;
}

void MultiSequence::toFiles(const std::string &baseName, bool is4bit) const {
  memoryToBinaryFile( ends.begin(), ends.end(), baseName + ".ssp" );

  memoryToBinaryFile( seq.begin(),
		      seq.begin() + (ends.back() + is4bit) / (is4bit + 1),
		      baseName + ".tis" );

  memoryToBinaryFile( nameEnds.begin(), nameEnds.begin() + ends.size(),
		      baseName + ".sds" );

  memoryToBinaryFile( names.begin(),
		      names.begin() + nameEnds[ finishedSequences() ],
		      baseName + ".des" );

  memoryToBinaryFile( qualityScores.begin(),
                      qualityScores.begin() + ends.back() * qualsPerLetter(),
                      baseName + ".qua" );
}

void MultiSequence::readFastxName(std::istream& stream) {
  std::string line, a;
  getline(stream, line);
  if (!stream) return;
  std::istringstream iss(line);
  iss >> a;
  addName(a);
}

std::istream&
MultiSequence::appendFromFasta(std::istream &stream, size_t maxSeqLen,
			       bool isCirc) {
  if( isFinished() ){
    char c = '>';
    stream >> c;
    if( c != '>' )
      throw std::runtime_error("bad FASTA sequence data: missing '>'");
    readFastxName(stream);
    if( !stream ) return stream;
  }

  std::streambuf *buf = stream.rdbuf();
  int c = buf->sgetc();

  while (c != std::streambuf::traits_type::eof()) {
    if (c > ' ') {  // faster than isspace
      if (c == '>' || seq.v.size() >= maxSeqLen) break;
      seq.v.push_back(c);
    }
    c = buf->snextc();
  }

  if (isRoomToFinish(maxSeqLen, isCirc)) finishTheLastSequence(isCirc);

  return stream;
}

void MultiSequence::reverseComplementOneSequence(size_t seqNum,
						 const uchar *complement) {
  size_t b = seqBeg(seqNum);
  size_t e = seqEnd(seqNum);
  uchar *s = seqWriter();
  std::reverse(s + b, s + e);
  reverse(qualityScores.v.begin() + b * qualsPerLetter(),
	  qualityScores.v.begin() + e * qualsPerLetter());

  if (complement) {
    for (size_t i = b; i < e; ++i) {
      s[i] = complement[s[i]];
    }
    char &strandChar = names.v[nameEnds.v[seqNum + 1] - 1];
    strandChar = ((strandChar - 1) ^ 1) + 1;
  }

  if (!pssm.empty()) {
    int *p = &pssm[0];
    while (b < e) {
      --e;
      for (unsigned i = 0; i < scoreMatrixRowSize; ++i) {
	unsigned j = complement ? complement[i] : i;
	if (b < e || i < j) std::swap(p[b * scoreMatrixRowSize + i],
				      p[e * scoreMatrixRowSize + j]);
      }
      ++b;
    }
  }
}

void MultiSequence::duplicateOneSequence(size_t seqNum) {
  size_t nameBeg = nameEnds[seqNum];
  size_t nameEnd = nameEnds[seqNum + 1];
  for (size_t i = nameBeg; i < nameEnd; ++i) {
    names.v.push_back(names.v[i]);
  }
  nameEnds.v.push_back(names.v.size());

  size_t b = seqBeg(seqNum);
  size_t e = padEnd(seqNum);

  for (size_t i = b; i < e; ++i) {
    seq.v.push_back(seq.v[i]);
  }
  ends.v.push_back(seq.v.size());

  for (size_t i = b * qualsPerLetter(); i < e * qualsPerLetter(); ++i) {
    qualityScores.v.push_back(qualityScores.v[i]);
  }

  assert(pssm.empty());  // implement this if & when needed
}

}
