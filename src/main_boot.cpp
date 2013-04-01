/*
 Bare-bones sequence alignment resampling. Default is bootstrap, alternative is joackknife.
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <getopt.h>

using namespace std;

#include "utils.h"
#include "sequence.h"
#include "seq_reader.h"
#include "boot.h"

void print_help(){
    cout << "Sequence alignment bootstrap or jackknife resampling" << endl;
    cout << "This will take fasta, fastq, phylip, and nexus inputs." << endl;
    cout << endl;
    cout << "Usage: pxboot [OPTION]... " << endl;
    cout << endl;
    cout << " -s, --seqf=FILE     input sequence file, stdin otherwise" << endl;
    cout << " -o, --outf=FILE     output sequence file, stout otherwise" << endl;
    cout << " -f, --frac=DOUBLE   jackknife percentage, default bootstrap" << endl;
    cout << " -x, --seed=INT      random number seed, clock otherwise" << endl;
    cout << "     --help          display this help and exit" << endl;
    cout << "     --version       display version and exit" << endl;
    cout << endl;
    cout << "Report bugs to: <https://github.com/FePhyFoFum/phyx/issues>" << endl;
    cout << "phyx home page: <https://github.com/FePhyFoFum/phyx>" << endl;
}

string versionline("pxseqsamp 0.1\nCopyright (C) 2013 FePhyFoFum\nLicense GPLv2\nwritten by Joseph W. Brown, Stephen A. Smith (blackrim)");

static struct option const long_options[] =
{
    {"seqf", required_argument, NULL, 's'},
    {"outf", required_argument, NULL, 'o'},
    {"frac", required_argument, NULL, 'f'},
    {"seed", required_argument, NULL, 'x'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0}
};

int main(int argc, char * argv[]) {
    bool going = true;
    bool outfileset = false;
    bool fileset = false;
    float jackfract = 0.0;
    int numchar = 0;
    char * outf;
    char * seqf;
    int seed = -1;
    while (going) {
        int oi = -1;
        int c = getopt_long(argc, argv, "s:o:f:x:hV", long_options, &oi);
        if (c == -1) {
            break;
        }
        switch(c) {
            case 's':
                fileset = true;
                seqf = strdup(optarg);
                break;
            case 'o':
                outfileset = true;
                outf = strdup(optarg);
                break;
            case 'f':
                jackfract = atof(strdup(optarg));
                if (jackfract < 0 || jackfract > 1) {
                    cout << "Jackknife fraction must be 0 < x < 1" << endl;
                    exit(0);
                }
                break;
            case 'x':
                seed = atof(strdup(optarg));
                break;
            case 'h':
                print_help();
                exit(0);
            case 'V':
                cout << versionline << endl;
                exit(0);
            default:
                print_error(argv[0],(char)c);
                exit(0);
        }
    }
    
    istream* pios;
    ostream* poos;
    ifstream* fstr;
    ofstream* ofstr;
    
    if (outfileset == true) {
        ofstr = new ofstream(outf);
        poos = ofstr;
    } else {
        poos = &cout;
    }
    
    if (fileset == true) {
        fstr = new ifstream(seqf);
        pios = fstr;
    } else {
        pios = &cin;
    }
    
    SequenceSampler ss(seed, jackfract);
    
    Sequence seq;
    string retstring;
    bool first = true;
    
    int ft = test_seq_filetype_stream(*pios, retstring);
    
    while (read_next_seq_from_stream(*pios, ft, retstring, seq)) {
        if (first) { // need to read in first sequence to get numchar
            numchar = (int)seq.get_sequence().size();
            ss.sample_sites(numchar);
            first = false;
        }
        (*poos) << ">" << seq.get_id() << endl;
        (*poos) << ss.get_resampled_seq(seq.get_sequence()) << endl;
    }
    
// have to deal with last sequence outside while loop. fix this.
    (*poos) << ">" << seq.get_id() << endl;
    (*poos) << ss.get_resampled_seq(seq.get_sequence()) << endl;
    
    if (fileset) {
        fstr->close();
        delete pios;
    }
    if (outfileset) {
        ofstr->close();
        delete poos;
    }
}