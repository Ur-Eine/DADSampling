#define EIGEN_DONT_PARALLELIZE
#define EIGEN_DONT_VECTORIZE
#define COUNT_DIMENSION
// #define COUNT_DIST_TIME

#include <iostream>
#include <fstream>

#include <ctime>
#include <cmath>
#include <matrix.h>
#include <utils.h>
#include <ivf/ivf.h>
#include <dade.h>
#include <getopt.h>

using namespace std;

const int MAXK = 100;

long double rotation_time=0;

void test(const Matrix<float> &Q, const Matrix<unsigned> &G, const IVF &ivf, int k){
    float sys_t, usr_t, usr_t_sum = 0, total_time=0, search_time=0;
    struct rusage run_start, run_end;

    vector<int> nprobes;
    for (int i=1; i<=20; ++i){
        nprobes.push_back(20*i);
    }
    
    for(auto nprobe:nprobes){
        total_time=0;
        dade::clear();
        int correct = 0;

        for(int i=0;i<Q.n;i++){
            GetCurTime( &run_start);
            ResultHeap KNNs = ivf.search(Q.data + i * Q.d, k, nprobe);
            GetCurTime( &run_end);
            GetTime(&run_start, &run_end, &usr_t, &sys_t);
            total_time += usr_t * 1e6;
            // Recall
            while(KNNs.empty() == false){
                int id = KNNs.top().second;
                KNNs.pop();
                for(int j=0;j<k;j++)
                    if(id == G.data[i * G.d + j])correct ++;
            }
        }
        float time_us_per_query = total_time / Q.n + rotation_time;
        float recall = 1.0f * correct / (Q.n * k);
        
        // (Search Parameter, Recall, Average Time/Query(us), Total Dimensionality)
        cout << nprobe << " " << recall * 100.00 << " " << time_us_per_query << " " << dade::tot_dimension << endl;
    }
}

int main(int argc, char * argv[]) {

    const struct option longopts[] ={
        // General Parameter
        {"help",                        no_argument,       0, 'h'}, 

        // Query Parameter 
        {"randomized",                  required_argument, 0, 'd'},
        {"K",                           required_argument, 0, 'k'},
        {"epsilon0",                    required_argument, 0, 'e'},
        {"delta_d",                     required_argument, 0, 'p'},

        // Indexing Path 
        {"dataset",                     required_argument, 0, 'n'},
        {"index_path",                  required_argument, 0, 'i'},
        {"query_path",                  required_argument, 0, 'q'},
        {"groundtruth_path",            required_argument, 0, 'g'},
        {"result_path",                 required_argument, 0, 'r'},
        {"transformation_path",         required_argument, 0, 't'},
        {"eigenvalue_path",             required_argument, 0, 'l'},
        {"epsilon_path",                required_argument, 0, 's'},
    };

    int ind;
    int iarg = 0;
    opterr = 1;    //getopt error message (off: 0)

    char index_path[256] = "";
    char query_path[256] = "";
    char groundtruth_path[256] = "";
    char result_path[256] = "";
    char dataset[256] = "";
    char transformation_path[256] = "";
    char eigenvalue_path[256] = "";
    char epsilon_path[256] = "";

    int randomize = 0;
    int subk = 0;

    while(iarg != -1){
        iarg = getopt_long(argc, argv, "d:i:q:g:r:t:n:k:e:p:l:s:", longopts, &ind);
        switch (iarg){
            case 'd':
                if(optarg)randomize = atoi(optarg);
                break;
            case 'k':
                if(optarg)subk = atoi(optarg);
                break;  
            case 'e':
                if(optarg)dade::epsilon0 = atof(optarg);
                break;
            case 'p':
                if(optarg)dade::delta_d = atoi(optarg);
                break;              
            case 'i':
                if(optarg)strcpy(index_path, optarg);
                break;
            case 'q':
                if(optarg)strcpy(query_path, optarg);
                break;
            case 'g':
                if(optarg)strcpy(groundtruth_path, optarg);
                break;
            case 'r':
                if(optarg)strcpy(result_path, optarg);
                break;
            case 't':
                if(optarg)strcpy(transformation_path, optarg);
                break;
            case 'l':
                if(optarg)strcpy(eigenvalue_path, optarg);
                break;
            case 's':
                if(optarg)strcpy(epsilon_path, optarg);
                break;
            case 'n':
                if(optarg)strcpy(dataset, optarg);
                break;
        }
    }
    
    Matrix<float> Q(query_path);
    Matrix<unsigned> G(groundtruth_path);
    Matrix<float> P(transformation_path);
    Matrix<float> L(eigenvalue_path);
    Matrix<float> E(epsilon_path);
    
    freopen(result_path,"a",stdout);
    if(randomize){
        StopW stopw = StopW();
        Q = mul(Q, P);
        rotation_time = stopw.getElapsedTimeMicro() / Q.n;
        dade::D = Q.d;
        if(randomize == 3 || randomize == 4){
            dade::epsilon.push_back(1.0e10);
            for(int i=0; i<Q.d; ++i){
                dade::lmds.push_back(L.data[0*L.d+i]);
                dade::epsilon.push_back(E.data[0*L.d+i]);
            }dade::USE_PCA = true;
            dade::compute_cdf_lmd();
            randomize -= 2;
        }
    }
    
    IVF ivf;
    ivf.load(index_path);
    test(Q, G, ivf, subk);
    return 0;
}
