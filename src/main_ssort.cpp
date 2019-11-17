#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <getopt.h>

#include "seq_reader.h"
#include "sequence.h"
#include "seq_utils.h"
#include "utils.h"
#include "log.h"
#include "edlib.h"

void print_help() {
    std::cout << "Sort sequences by id or length" << std::endl;
    std::cout << "Can read from stdin or file, but output is fasta." << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: pxssort [OPTION]... [FILE]..."<<std::endl;
    std::cout << std::endl;
    std::cout << " -s, --seqf=FILE     input sequence file, stdin otherwise"<<std::endl;
    std::cout << " -b, --sortby        what to sort by: 1:id (default) 2:id rev" << std::endl;
    std::cout << "                                      3:length (<)   4:length (>)" << std::endl;
    std::cout << " -o, --outf=FILE     output sequence file, stout otherwise"<<std::endl;
    std::cout << " -h, --help          display this help and exit"<<std::endl;
    std::cout << " -V, --version       display version and exit"<<std::endl;
    std::cout << std::endl;
    std::cout << "Report bugs to: <https://github.com/FePhyFoFum/phyx/issues>" <<std::endl;
    std::cout << "phyx home page: <https://github.com/FePhyFoFum/phyx>"<<std::endl;
}

std::string versionline("pxssort 0.1\nCopyright (C) 2017 FePhyFoFum\nLicense GPLv3\nwritten by Stephen A. Smith (blackrim)");

static struct option const long_options[] =
{
    {"seqf", required_argument, NULL, 's'},
    {"sortby", required_argument, NULL, 'b'},
    {"outf", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0}
};

struct SequenceIDListCompare {
    bool operator()(const Sequence & lhs, const Sequence & rhs) {
      return lhs.get_id() < rhs.get_id();
    }
} SequenceIDListCompare;

struct SequenceRevIDListCompare {
    bool operator()(const Sequence & lhs, const Sequence & rhs) {
      return lhs.get_id() > rhs.get_id();
    }
} SequenceRevIDListCompare;

struct SequenceLengthListCompare {
    bool operator()(const Sequence & lhs, const Sequence & rhs) {
      return lhs.get_sequence().length() < rhs.get_sequence().length();
  }
} SequenceLengthListCompare;

struct SequenceRevLengthListCompare {
    bool operator()(const Sequence & lhs, const Sequence & rhs) {
      return lhs.get_sequence().length() > rhs.get_sequence().length();
  }
} SequenceRevLengthListCompare;

int main(int argc, char * argv[]) {
    
    log_call(argc, argv);
    
    bool fileset = false;
    bool outfileset = false;
    int sortby = 1;
    
    char * seqf = NULL;
    char * outf = NULL;
    while (1) {
        int oi = -1;
        int c = getopt_long(argc, argv, "s:b:o:hgV", long_options, &oi);
        if (c == -1) {
            break;
        }
        switch(c) {
            case 's':
                fileset = true;
                seqf = strdup(optarg);
                check_file_exists(seqf);
                break;
            case 'b':
                sortby = string_to_int(optarg, "-b");
                break;
            case 'o':
                outfileset = true;
                outf = strdup(optarg);
                break;
            case 'h':
                print_help();
                exit(0);
            case 'V':
                std::cout << versionline << std::endl;
                exit(0);
            default:
                print_error(argv[0], (char)c);
                exit(0);
        }
    }
    
    if (fileset && outfileset) {
        check_inout_streams_identical(seqf, outf);
    }
    
    std::istream * pios = NULL;
    std::ostream * poos = NULL;
    std::ifstream * fstr = NULL;
    std::ofstream * ofstr = NULL;
    
    if (fileset == true) {
        fstr = new std::ifstream(seqf);
        pios = fstr;
    } else {
        pios = &std::cin;
        if (check_for_input_to_stream() == false) {
            print_help();
            exit(1);
        }
    }
    if (outfileset == true) {
        ofstr = new std::ofstream(outf);
        poos = ofstr;
    } else {
        poos = &std::cout;
    }
    std::vector<Sequence> seqs;
    Sequence seq;
    std::string retstring;
    int ft = test_seq_filetype_stream(*pios,retstring);
    while (read_next_seq_from_stream(*pios,ft,retstring,seq)) {
        seqs.push_back(seq);
    }
    if (ft == 2) {
        seqs.push_back(seq);
    }
    if (sortby == 1) {
        sort(seqs.begin(),seqs.end(),SequenceIDListCompare);
    } else if (sortby == 2) {
        sort(seqs.begin(),seqs.end(),SequenceRevIDListCompare);
    } else if (sortby == 3) {
        sort(seqs.begin(),seqs.end(),SequenceLengthListCompare);
    } else if (sortby == 4) {
        sort(seqs.begin(),seqs.end(),SequenceRevLengthListCompare);
    }
    for (unsigned int i=0;i<seqs.size();i++) {
        (*poos) << seqs[i].get_fasta();
    }

    if (fileset) {
        fstr->close();
        delete pios;
    }
    if (outfileset) {
        ofstr->close();
        delete poos;
    }
    return EXIT_SUCCESS;
}
