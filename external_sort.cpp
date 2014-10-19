#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <iostream>
using namespace std;


void load_data(std::vector<int>& recs, std::ifstream& in, size_t num_to_load) {
    recs.clear();
    recs.reserve(num_to_load);
    size_t count = 0;
    while (count < num_to_load) {
        int v = 0;
        in >> v;
        if (!in.good()) {
            break;
        }
        recs.push_back(v);
        ++count;
    }
}

void save_data(const std::vector<int>& recs, std::ofstream& out) {
    std::vector<int>::const_iterator first = recs.begin(), last = recs.end();
    while (first != last) {
        out << *first << endl;
        ++first;
    }
}

void gen_test_data(const std::string& fname, size_t num_data) {
    std::ofstream out(fname);
    while (num_data > 0) {
        out << rand() << std::endl;
        --num_data;
    }
    out.close();
}

template<typename T>
class ExternalSort {
    int batch_size_;
    typedef std::vector<T> Vector;
    typedef std::vector<std::string> VectorS;

    VectorS tmp_fnames_;

    std::string next_tmp_fname() {
        char buf[100];
        sprintf(buf, tmp_template_, tmp_counter_++);
        return std::string(buf);
    }

    size_t tmp_counter_;
    const static char* tmp_template_; 

    void split_and_sort(const std::string& in_file_name) {
        ifstream in(in_file_name);
        while (true) {
            Vector recs;
            load_data(recs, in, batch_size_);
            if (recs.empty()) {
                break;
            }
            std::cout << "recs.size: " << recs.size() << std::endl;
            std::sort(recs.begin(), recs.end());
            std::string tmp_name = next_tmp_fname();
            tmp_fnames_.push_back(tmp_name);

            std::ofstream out(tmp_name);
            save_data(recs, out); 
            out.close();
        }
        in.close();
    }

public:
    ExternalSort(int batch_size) : tmp_counter_(0), batch_size_(batch_size) {
    }

    void sort_file(const std::string& out_file_name, const std::string& in_file_name) {
        split_and_sort(in_file_name); 
        // {
        //     VectorS::const_iterator first = tmp_fnames_.begin(), last = tmp_fnames_.end();
        //     while (first != last) {
        //         std::cout << *first << std::endl;
        //         ++first;
        //     }
        // }

        VectorS merged_files(1000, "");
        VectorS::const_iterator first = tmp_fnames_.begin(), last = tmp_fnames_.end();
        while (first != last) {
            std::string carry = *first; 
            size_t pos = 0;
            while (true) {
                if (merged_files[pos].empty()) {
                    merged_files[pos] = carry;
                    break;
                }
                std::string merged_fname = next_tmp_fname();
                merge_sorted_file(merged_fname, carry, merged_files[pos]);
                std::remove(merged_files[pos].c_str());
                merged_files[pos] = "";
                std::remove(carry.c_str());
                carry = merged_fname;
                ++pos;
                std::cout << *first << ", " << pos << std::endl;
            }
            ++first;
        }

        {
            std::cout << merged_files.size() << std::endl;
            VectorS::const_iterator first = merged_files.begin(), last = merged_files.end();
            while (first != last) {
                std::cout << *first << std::endl;
                ++first;
            }
        }
        first = merged_files.begin(), last = merged_files.end();
        std::string res = "";
        while (first != last) {
            std::string merged_fname = next_tmp_fname();
            if (!first->empty()) {
                if (res.empty()) {
                    res = *first;
                } else {
                    merge_sorted_file(merged_fname, *first, res);
                    std::remove(res.c_str());
                    std::remove(first->c_str());
                    res = merged_fname;
                }
            }
            ++first;
        }
        std::rename(res.c_str(), out_file_name.c_str());
    }

    void merge_sorted_file(const std::string& out_file, const std::string& in_file_1, const std::string& in_file_2);

    void merge_sorted_vector(Vector& vec, const Vector& vec1, const Vector& vec2);

};

template<typename T>
const char* ExternalSort<T>::tmp_template_ = "msort_tmp_file_%d.txt"; 

template<typename T>
void ExternalSort<T>::merge_sorted_vector(Vector& vec, const Vector& vec1, const Vector& vec2) {
    vec.clear();
    typename Vector::const_iterator first1 = vec1.begin(), last1 = vec1.end();
    typename Vector::const_iterator first2 = vec2.begin(), last2 = vec2.end();
    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) {
            vec.push_back(*first1);
            ++first1;
        } else {
            vec.push_back(*first2);
            ++first2;
        }
    }
    while (first1 != last1) {
        vec.push_back(*first1);
        ++first1;
    }
    while (first2 != last2) {
        vec.push_back(*first2);
        ++first2;
    }
}

template<typename T>
void ExternalSort<T>::merge_sorted_file(const std::string& out_file, const std::string& in_file_1, const std::string& in_file_2) {
    std::ifstream in1(in_file_1), in2(in_file_2);
    std::ofstream out(out_file);
    Vector recs1, recs2, recs;
    while (true) {
        load_data(recs1, in1, batch_size_);
        load_data(recs2, in2, batch_size_);
        merge_sorted_vector(recs, recs1, recs2); 
        if (recs.empty()) {
            break;
        }
        save_data(recs, out);
    }
    out.close();
    in1.close();
    in2.close();
}

int main() {
    std::string in_fname = "msort_dat.txt";
    std::string out_fname = "msort_result.txt";
    // gen_test_data(in_fname, 1024);
    gen_test_data(in_fname, 1536);
    int batch_size = 512;
    ExternalSort<int> ext_sort(batch_size);
    ext_sort.sort_file(out_fname, in_fname);
    return 0; 
}