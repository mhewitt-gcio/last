// Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014 Martin C. Frith

#include "LastdbArguments.hh"
#include "stringify.hh"
#include "getoptUtil.hh"
#include <iostream>
#include <stdexcept>
#include <cstdlib>  // EXIT_SUCCESS
#include <cstring>  // strtok

#define ERR(x) throw std::runtime_error(x)

static void badopt( char opt, const char* arg ){
  ERR( std::string("bad option value: -") + opt + ' ' + arg );
}

static void badopt(const char *opt, const char *arg) {
  ERR(std::string("bad option value: --") + opt  + '=' + arg);
}

using namespace cbrc;

LastdbArguments::LastdbArguments() :
  isProtein(false),
  isAddStops(false),
  isCircular(false),
  isKeepLowercase(false),
  tantanSetting(-1),  // the default depends on other parameters
  maxRepeatUnit(-1),  // the default depends on other parameters
  isCaseSensitive(false),
  seedPatterns(0),
  strand(1),
  volumeSize(-1),
  indexStep(1),
  minimizerWindow(1),
  numOfThreads(1),
  subsetSeedFile(""),
  userAlphabet(""),
  minSeedLimit(0),
  bucketDepth(-1),  // means: use the default (adapts to the data)
  minIndexedPositionsPerBucket(4),
  childTableType(0),
  isCountsOnly(false),
  isDump(false),
  verbosity(0),
  inputFormat(sequenceFormat::fasta),
  bitsPerBase(8){}

void LastdbArguments::fromArgs( int argc, char** argv, bool isOptionsOnly ){
  programName = argv[0];
  std::string usage = "\
Usage: " + std::string(programName) +
    " [options] output-name fasta-sequence-file(s)\n\
Prepare sequences for subsequent alignment with lastal.\n\
\n\
Main Options:\n\
 -h, --help  show all options and their default settings, and exit\n\
 -p  interpret the sequences as proteins\n\
 -c  soft-mask lowercase letters (in reference *and* query sequences)\n\
 -u  seeding scheme (default: YASS if DNA, else PSEUDO if -q, else exact-match)\n\
 -P  number of parallel threads (default: " + stringify(numOfThreads) + ")";

  std::string help = usage + "\n\
\n\
Advanced Options (default settings):\n\
 -q  interpret the sequences as proteins and append */STOP\n\
 -S  strand: 0=reverse, 1=forward, 2=both (default: "
    + stringify(strand) + ")\n\
 -R  lowercase & simple-sequence options (default: 03 for -q, else 01)\n\
 -U  maximum tandem repeat unit length (protein: 50, DNA: 100 or 400)\n\
 -w  use initial matches starting at every w-th position in each sequence ("
    + stringify(indexStep) + ")\n\
 -W  use \"minimum\" positions in sliding windows of W consecutive positions ("
    + stringify(minimizerWindow) + ")\n\
 -Q  input format: fastx, keep, sanger, solexa, illumina (default: fasta)\n\
 -s  volume size (default: unlimited)\n\
 -m  seed patterns (1=match, 0=anything, @=transition)\n\
 -d  DNA seed patterns (N=match, n=anything, R=purine match, etc.)\n\
 -a  user-defined alphabet\n\
 -i  minimum limit on initial matches per query position (default: "
    + stringify(minSeedLimit) + ")\n\
 -b  maximum length for buckets\n\
 -B  use max bucket length with memory <= (memory for stored positions) / B ("
    + stringify(minIndexedPositionsPerBucket) + ")\n\
 -C  child table type: 0=none, 1=byte-size, 2=short-size, 3=full (default: "
    + stringify(childTableType) + ")\n\
 -x  just count sequences and letters\n\
 -D  print all sequences in lastdb files\n\
 --bits=N  use this many bits per base for DNA sequence (default: "
    + stringify(bitsPerBase) + ")\n\
 --circular  these sequences are circular\n\
 -v  be verbose: write messages about what lastdb is doing\n\
 -V, --version  show version information, and exit\n\
";

  static const char sOpts[] = "hVpqR:U:cm:d:S:s:w:W:P:u:a:i:b:B:C:xDvQ:";

  static struct option lOpts[] = {
    { "help",    no_argument, 0, 'h' },
    { "version", no_argument, 0, 'V' },
    { "bits",    required_argument, 0, 128 },
    { "circular", no_argument, 0, 'C' - 'A' },
    { 0, 0, 0, 0 }
  };

  int c, optionIndex;
  while ((c = getopt_long(argc, argv, sOpts, lOpts, &optionIndex)) != -1) {
    switch(c){
    case 'h':
      std::cout << help;
      throw EXIT_SUCCESS;
    case 'V':
      std::cout << "lastdb "
#include "version.hh"
	"\n";
      throw EXIT_SUCCESS;
    case 'p':
      isProtein = true;
      break;
    case 'q':
      isAddStops = true;
      break;
    case 'R':
      if( optarg[0] < '0' || optarg[0] > '1' ) badopt( c, optarg );
      if( optarg[1] < '0' || optarg[1] > '3' ) badopt( c, optarg );
      if( optarg[2] ) badopt( c, optarg );
      isKeepLowercase = optarg[0] - '0';
      tantanSetting = optarg[1] - '0';
      break;
    case 'U':
      unstringify(maxRepeatUnit, optarg);
      if (maxRepeatUnit < 0) badopt(c, optarg);
      break;
    case 'c':
      isCaseSensitive = true;
      break;
    case 'm':
      seedPatterns.push_back(optarg);
      break;
    case 'd':
      dnaSeedPatterns.push_back(optarg);
      break;
    case 'S':
      unstringify( strand, optarg );
      if( strand < 0 || strand > 2 ) badopt( c, optarg );
      break;
    case 's':
      unstringifySize( volumeSize, optarg );
      break;
    case 'w':
      unstringify( indexStep, optarg );
      if( indexStep < 1 ) badopt( c, optarg );
      break;
    case 'W':
      unstringify( minimizerWindow, optarg );
      if( minimizerWindow < 1 ) badopt( c, optarg );
      break;
    case 'P':
      unstringify( numOfThreads, optarg );
      break;
    case 'u':
      subsetSeedFile = optarg;
      break;
    case 'a':
      userAlphabet = optarg;
      break;
    case 'i':
      unstringify( minSeedLimit, optarg );
      break;
    case 'b':
      unstringify( bucketDepth, optarg );
      break;
    case 'B':
      unstringify( minIndexedPositionsPerBucket, optarg );
      if ( minIndexedPositionsPerBucket == 0 ) badopt( c, optarg );
      break;
    case 'C':
      unstringify( childTableType, optarg );
      if( childTableType < 0 || childTableType > 3 ) badopt( c, optarg );
      break;
    case 'x':
      isCountsOnly = true;
      break;
    case 'D':
      isDump = true;
      break;
    case 'v':
      ++verbosity;
      break;
    case 'Q':
      unstringify( inputFormat, optarg );
      if (inputFormat == sequenceFormat::prb ||
	  inputFormat == sequenceFormat::pssm) badopt(c, optarg);
      break;
    case 'C' - 'A':
      isCircular = true;
      break;
    case 128:
      unstringify(bitsPerBase, optarg);
      if (bitsPerBase != 4 &&
	  bitsPerBase != 8) badopt(lOpts[optionIndex].name, optarg);
      break;
    case '?':
      ERR( "bad option" );
    }
  }

  if (bitsPerBase < 8 && (!userAlphabet.empty() || isProtein || isAddStops)) {
    ERR("can't use --bits=4 with non-default alphabet");
  }

  if (tantanSetting > 0 && maxRepeatUnit == 0) {
    ERR("can't find repeats with maximum unit length 0");
  }

  if (tantanSetting == 2 && (isProtein || isAddStops)) {
    ERR("can't find DNA repeats in protein");
  }

  if( !isOptionsOnly ){
    if( optind >= argc )
      ERR( "please give me an output name and sequence file(s)\n\n" + usage );
    lastdbName = argv[optind++];
    inputStart = optind;
  }

  resetGetopt();
}

void LastdbArguments::fromLine( const std::string& line ){
  const char* delimiters = " \t";
  const char* s = line.c_str();
  std::vector<char> args( s, s + line.size() + 1 );
  std::vector<char*> argv;
  char* i = std::strtok( &args[0], delimiters );
  argv.push_back(i);
  while( i ){
    i = std::strtok( 0, delimiters );
    argv.push_back(i);
  }
  fromArgs( argv.size() - 1, &argv[0], true );
}

void LastdbArguments::fromString( const std::string& s ){
  std::string trigger = "#lastdb";
  std::istringstream iss(s);
  std::string line;
  while( getline( iss, line ) )
    if( line.compare( 0, trigger.size(), trigger ) == 0 )
      fromLine( line );
}
