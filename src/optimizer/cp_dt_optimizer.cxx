
// #include "decomposition.h"
#include <ctf.hpp>
#include "../../common.h"


using namespace CTF;

template<typename dtype>
CPDTOptimizer<dtype>::CPDTOptimizer(int order, int r, World & dw)
    : CPOptimizer<dtype>(order, r, dw){

    // make the char seq_V
    seq_V[order] = '\0';
    seq_tree_top[order] = '\0';
    for (int j=0; j<order; j++) {
        seq_V[j] = 'a'+j;
        seq_tree_top[j] = seq_V[j];
    }
    seq_tree_top[order-1] = '*';

    // construct the tree
    Construct_Dimension_Tree();

    // initialize the indexes
    indexes = vector<int>(order-1, 0);
    for (int i=0; i<indexes.size(); i++) {
        indexes[i] = i;
    }
    indexes1 = indexes;
    indexes2 = indexes1;

    left_index = order-1;
    left_index1 = left_index;
    left_index2 = (left_index + order - 1) % order;
    update_indexes(indexes2, left_index2);
    special_index = 0;

    first_subtree = true;
}

template<typename dtype>
CPDTOptimizer<dtype>::~CPDTOptimizer(){
    // delete S;
}

template<typename dtype>
void CPDTOptimizer<dtype>::update_left_index(){
  int order = this->order;
  left_index = (left_index + order - 1) % order;
}

template<typename dtype>
void CPDTOptimizer<dtype>::update_indexes(vector<int> &indexes, int left_index) {
    int order = this->order;

    int j = 0;
    for (int i=left_index+1; i<order; i++) {
        indexes[j] = i;
        j++;
    }
    for (int i=0; i<left_index; i++) {
        indexes[j] = i;
        j++;
    }
}

template<typename dtype>
void CPDTOptimizer<dtype>::vec2str(vector<int> vec, string & seq_out) {
    char seq[vec.size()+2];
    seq[vec.size()+1] = '\0';
    seq[vec.size()] = '*';
    for (int i=0; i<vec.size(); i++) {
        seq[i] = 'a' + vec[i];
    }
    seq_out = seq;
}

template<typename dtype>
void CPDTOptimizer<dtype>::Construct_Dimension_Tree() {
    int order = this->order;
    vector<int> top_node = vector<int>(order-1);
    for (int i=0; i<top_node.size(); i++) {
        top_node[i] = i;
    }

    Construct_Subtree(top_node);
}

template<typename dtype>
void CPDTOptimizer<dtype>::Construct_Subtree(vector<int> top_node) {
    Right_Subtree(top_node);

    vector<int> child_node = vector<int>(top_node.size()-1);
    for (int i=0; i<child_node.size(); i++) {
        child_node[i] = top_node[i];
    }

    vector<int> mat_index = {top_node[top_node.size()-1]};

    string child_seq, top_seq, mat_seq;
    vec2str(child_node, child_seq);
    vec2str(top_node, top_seq);
    vec2str(mat_index, mat_seq);

    parent[child_seq] = top_seq;
    contract_index[child_seq] = mat_seq;

    if (child_node.size()>1) {
        Construct_Subtree(child_node);
    }

}

template<typename dtype>
void CPDTOptimizer<dtype>::Right_Subtree(vector<int> top_node) {
    // construct the right tree
    vector<int> child_node = vector<int>(top_node.size()-1);
    for (int i=0; i<child_node.size(); i++) {
        child_node[i] = top_node[i];
    }
    child_node[child_node.size()-1] = top_node[top_node.size()-1];

    vector<int> mat_index = {top_node[top_node.size()-2]};

    string child_seq, top_seq, mat_seq;
    vec2str(child_node, child_seq);
    vec2str(top_node, top_seq);
    vec2str(mat_index, mat_seq);

    parent[child_seq] = top_seq;
    contract_index[child_seq] = mat_seq;

    if (child_node.size()>1) {
        Right_Subtree(child_node);
    }
}

template<typename dtype>
void CPDTOptimizer<dtype>::mttkrp_map_init(int left_index) {
    World * dw = this->world;
    int order = this->order;

    // build seq_map_init
    seq_map_init[order] = '\0';
    seq_map_init[order-1] = '*';
    int j = 0;
    for (int i=left_index+1; i<order; i++){
        seq_map_init[j] = 'a' + i;
        j++;
    }
    for (int i=0; i<left_index; i++) {
        seq_map_init[j] = 'a' + i;
        j++;
    }
    // build seq_matrix
    char seq_matrix[3];
    seq_matrix[2] = '\0';
    seq_matrix[1] = '*';
    seq_matrix[0] = 'a'+left_index;
    // store that into the mttkrp_map
    int lens[strlen(seq_map_init)];
    for (int ii=0; ii<strlen(seq_map_init); ii++){
        if (seq_map_init[ii] == '*') lens[ii] = this->W[0].ncol;
        else lens[ii] = this->V->lens[int(seq_map_init[ii]-'a')];
    }
    mttkrp_map[seq_tree_top] = Tensor<dtype>(strlen(seq_map_init), lens, *dw);

    mttkrp_map[seq_tree_top][seq_map_init] = (*this->V)[seq_V] * this->W[left_index][seq_matrix];

}

template<typename dtype>
void CPDTOptimizer<dtype>::mttkrp_map_DT(string index) {
    World * dw = this->world;
    char const * index_char = index.c_str();

    char const * parent_index = parent[index].c_str();
    if (mttkrp_map.find(parent_index)==mttkrp_map.end()) {
        mttkrp_map_DT(parent_index);
    }
    // get the modindexe of W
    char const * mat_index = contract_index[index].c_str();
    int W_index = int(mat_index[0] - 'a');
    int lens[strlen(index_char)];

    for (int ii=0; ii<strlen(index_char); ii++){
        if (index[ii] == '*') lens[ii] = this->W[0].ncol;
        else lens[ii] = this->V->lens[int(indexes[index[ii]-'a'])];
    }
    mttkrp_map[index] = Tensor<dtype>(strlen(index_char), lens, *dw);

    mttkrp_map[index][index_char] = mttkrp_map[parent_index][parent_index] * this->W[indexes[W_index]][mat_index];
}

template<typename dtype>
void CPDTOptimizer<dtype>::step() {

    World * dw = this->world;
    int order = this->order;

    if (first_subtree) {
        indexes = indexes1; 
        left_index = left_index1;
    }
    else {
        indexes = indexes2; 
        left_index = left_index2;
    }
    // clear the Hash Table
    mttkrp_map.clear();

    // reinitialize
    mttkrp_map_init(left_index);

    // iteration on W[i]
    for (int i=0; i<indexes.size(); i++) {

        if (first_subtree && i<special_index) 
            continue;
        if (!first_subtree && i>special_index) 
            break;
        /*  construct Matrix M
        *   M["dk"] = V["abcd"]*W1["ak"]*W2["bk"]*W3["ck"]
        */
        vector<int> mat_index = {i};

        string mat_seq;
        vec2str(mat_index, mat_seq);

        if (mttkrp_map.find(mat_seq)==mttkrp_map.end()) {
            mttkrp_map_DT(mat_seq);
        }
        Matrix<dtype> M = mttkrp_map[mat_seq];

        // calculating S
        CPOptimizer<dtype>::update_S(indexes[i]);
        // calculate gradient
        this->grad_W[indexes[i]]["ij"] = -M["ij"]+this->W[indexes[i]]["ik"]*this->S["kj"];

        SVD_solve(M, this->W[indexes[i]], this->S);
    }

    first_subtree = !first_subtree;
}
