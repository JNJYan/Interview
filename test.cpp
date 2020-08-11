/*
 * @Author: JNJYan
 * @LastEditors: JNJYan
 * @Email: jjy20140825@gmail.com
 * @Date: 2020-07-20 12:17:52
 * @LastEditTime: 2020-07-20 12:28:07
 * @Description: Modify here please
 * @FilePath: /Interview/test.cpp
 */ 
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

void maxHeap(vector<int>& nums, int pos, int n){
    int child = pos *2 +1;
    if(child >= n)
        return;
    if(child < n-1 && nums[child] < nums[child+1])
        child++;
    if(nums[pos] < nums[child]){
        swap(nums[pos], nums[child]);
        maxHeap(nums, child, n);
    }
}

void buildHeap(vector<int>& nums, int n){
    for(int i=n/2-1; i>=0; i--)
        maxHeap(nums, i, n);
}

void HeapSort(vector<int>& nums){
    int size = nums.size();
    buildHeap(nums, size);
    for(int i=size-1; i>0; ++i){
        swap(nums[0], nums[i]);
        maxHeap(nums, 0, i);
        
    }
}



int main(){
    return 0;
}