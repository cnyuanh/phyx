#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <fstream>

#include "seq_sample.h"
#include "utils.h"

SequenceSampler::SequenceSampler (const int& seed, const float& jackfract,
        std::string& partf):jkfract_(jackfract), jackknife_(false),
        partitioned_(false), num_partitioned_sites_(0), num_partitions_(0) {
    if (seed == -1) {
        srand(get_clock_seed());
    } else {
        srand(seed);
    }
    if (jkfract_ != 0.0) {
        jackknife_ = true;
    }
    if (!partf.empty()) {
        partitioned_ = true;
        parse_partitions(partf);
    }
}


// not used
std::vector<int> SequenceSampler::get_sampled_sites () {
    return sample_sites_;
}


std::string SequenceSampler::get_resampled_seq (const std::string& origseq) {
    std::string seq;
    for (unsigned int i = 0; i < sample_sites_.size(); i++) {
        if (i == 0) {
            seq = origseq[sample_sites_[i]];
        } else {
            seq += origseq[sample_sites_[i]];
        }
    }
    return seq;
}


// this is done once, to populate site sample vector
void SequenceSampler::sample_sites (const int& numchar) {
    if (partitioned_) {
        sample_sites_ = get_partitioned_bootstrap_sites();
    } else if (!jackknife_) {
        sample_sites_ = get_bootstrap_sites(numchar);
    } else {
        sample_sites_ = get_jackknife_sites(numchar);
    }
}


// sample with replacement.
std::vector<int> SequenceSampler::get_bootstrap_sites (const int& numchar) {
    std::vector<int> randsites (numchar); // numchar zero-initialized elements
    int randnum = 0;
    
    for (int i = 0; i < numchar; i++) {
        randnum = random_int_range(0, (numchar - 1));
        randsites[i] = randnum;
    }
    sort(randsites.begin(), randsites.end());
    
    return randsites;
}


// set up so same composition as original
std::vector<int> SequenceSampler::get_partitioned_bootstrap_sites () {
    std::vector<int> master(num_partitioned_sites_, 0);
    for (int i = 0; i < num_partitions_; i++) {
        int curNum = (int)partitions_[i].size();
        //std::cout << "Partition #" << i << " contains " << curNum << " sites." << std::endl;
        std::vector<int> randsites = get_bootstrap_sites(curNum);
        for (int j = 0; j < curNum; j ++) {
        // put partitions back in same spot as original, so partition file does not need to change
            master[partitions_[i][j]] = partitions_[i][randsites[j]];
        }
    }
    return master;
}

// sample WITHOUT replacement. not with partitioned models
std::vector<int> SequenceSampler::get_jackknife_sites (const int& numchar) {
    int numsample = numchar * jkfract_ + 0.5;
    std::vector<int> randsites = sample_without_replacement(numchar, numsample);
    return randsites;
}


/*
grab partition information from separate file. example:
mtDNA_1st = 1-2066\3
TGFb2 = 5059-5721
TODO: update to 1) RAxML and 2) Nexus style
 - RAxML: DNA, gene0 = 1-4243
 - MrBayes: CHARSET gene0 = 1-4243;
 * So, thrown out first token, and should work for both
*/
void SequenceSampler::parse_partitions (std::string& partf) {
    std::vector<int> temp;
    std::string line;
    std::ifstream infile(partf.c_str());
    
    while (getline(infile,line)) {
        if (line.size() < 1) {
            continue;
        } else {
            temp = get_partition_sites(line);
            partitions_.push_back(temp);
            temp.clear();
        }
    }
    infile.close();
    
    num_partitions_ = (int)partitions_.size();
    calculate_num_partitioned_sites();
    
    // do error-checking here:
    check_valid_partitions();
}


// expecting pattern: (CHARSET/DNA,) name = start-end[\3][,]
// want to be flexible with spaces/lackthereof
// guaranteed to be ordered
std::vector<int> SequenceSampler::get_partition_sites (const std::string& part) {
    std::vector<int> sites;
    std::vector<std::string> tokens;
    int start = 0;
    int stop = 0;
    int interval = 1;
    
    std::string delim(" -=;\t\\");
    tokenize(part, tokens, delim);
    get_partition_parameters (tokens, start, stop, interval);
    
    int i = start;
    while (i <= stop) {
        sites.push_back(i);
        i += interval;
    }
    return sites;
}


// opposite of get_partition_sites. want single vector listing site-specific partitions. e.g.
// 1231231231231234444444444444
// not used
void SequenceSampler::get_site_partitions () {
    std::vector<int> sites(num_partitioned_sites_, 0);
    for (int i = 0; i < num_partitions_; i++) {
        for (unsigned int j = 0; j < partitions_[i].size(); j++) {
            sites[partitions_[i][j]] = i;
        }
    }
    site_partitions_ = sites;
}


// CHARSET GADPH = 2991-3406\3
// after being tokenized, should be of length 4 or 5 (latter when interval)
// convert from 1-start to 0-start
void SequenceSampler::get_partition_parameters (std::vector<std::string>& tokens,
        int & start, int & stop, int & interval) {
    if ((int)tokens.size() < 4 || (int)tokens.size() > 5) {
        std::cout << "Error: invalid/unsupported partition specification." << std::endl;
        exit (0);
    }
    
    // ignore first token. will be either CHARSET of DNA,
    partition_names_.push_back(tokens[1]);
    //std::cout << "Processing partition '" << tokens[1] << "': ";
    
    if (is_number(tokens[2])) {
        start = atoi(tokens[2].c_str()) - 1;
        //std::cout << "start = " << start;
    }
    if (is_number(tokens[3])) {
        stop = atoi(tokens[3].c_str()) - 1;
        //std::cout << "; stop = " << stop;
    }
    if (((int)tokens.size() == 5) && is_number(tokens[4])) {
        interval = atoi(tokens[4].c_str());
        //std::cout << "; interval = " << interval;
    }
    //std::cout << std::endl;
}


void SequenceSampler::calculate_num_partitioned_sites () {
    num_partitioned_sites_ = 0;
    for (int i = 0; i < num_partitions_; i++) {
        num_partitioned_sites_ += (int)partitions_[i].size();
//         std::cout << "Partition #" << i << " contains " << (int)partitions[i].size() << " sites:" << std::endl;
//         for (unsigned int j = 0; j < partitions[i].size(); j++) {
//             std::cout << partitions[i][j] << " ";
//         }
//         std::cout << std::endl;
    }
}


int SequenceSampler::get_num_partitioned_sites () {
    return num_partitioned_sites_;
}


// should do some error-checking e.g. for 1) missing sites, 2) overlapping partitions
void SequenceSampler::check_valid_partitions () {
    std::vector<int> allSites = partitions_[0];
    for (int i = 1; i < num_partitions_; i++) {
        allSites.insert(allSites.end(), partitions_[i].begin(), partitions_[i].end());
    }
    sort(allSites.begin(), allSites.end());
    
    int max = allSites.back();
    int count = (int)allSites.size();
    int diff = max - count + 1;
    
    if (diff != 0) { // sites are duplicated
        //std::cout << "Error in partitioning: maximum site value " << max << " does not equal site count " << count << "." << std::endl;
        find_duplicates_missing(allSites);
    }
}


void SequenceSampler::find_duplicates_missing (const std::vector<int>& allSites) {
    std::vector<int> unique;
    std::vector<int> duplicates;
    std::vector<int> missing;
    
    int maxVal = allSites.back();
    
    std::vector<int> counts(maxVal, 0);
    for (unsigned int i = 0; i < allSites.size(); i++) {
        counts[allSites[i]]++;
    }
    
    for (int i = 0; i < maxVal; i++) {
        switch (counts[i]) {
            case 0:
                missing.push_back(i);
                break;
            case 1:
                unique.push_back(i);
                break;
            default:
                duplicates.push_back(i);
        }
    }
    
    if (duplicates.size() != 0) {
        std::cout << "Error: the following " << duplicates.size() << " sites are found in more than one partition: ";
        for (unsigned int i = 0; i < duplicates.size(); i++) {
            std::cout << duplicates[i] << " ";
        }
        std::cout << std::endl;
    }
    if (missing.size() != 0) {
        std::cout << "Error: the following " << missing.size() << " sites are not found in any partition: ";
        for (unsigned int i = 0; i < missing.size(); i++) {
            std::cout << missing[i] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Exiting." << std::endl;
    exit (0);
}
