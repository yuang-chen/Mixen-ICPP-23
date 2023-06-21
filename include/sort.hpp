#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#define SORT_HEADER

#define LSB_MASK 0x00000001

unsigned int computeMean (unsigned int a, unsigned int b)
{
    unsigned int meanVal = (a >> 1) + (b >> 1) + ((a & LSB_MASK) + (b & LSB_MASK))/2;
    return meanVal;
}

template <class T>
void mergeDescend (std::vector<T>& data, unsigned int low, unsigned int high, unsigned int mid)
{
    unsigned int i = low;
    unsigned int j = mid+1;
    unsigned int k = 0;
    
    std::vector<T> temp(high-low+1);
    while(i<=mid && j<=high)
    {
        if (data[i] >= data[j])
            temp[k++] = data[i++];
        else
            temp[k++] = data[j++];
    }
    while(i<=mid)
        temp[k++] = data[i++];
    while(j<=high)
        temp[k++] = data[j++];
    for (i=low; i<=high; i++)
        data[i] = temp[i-low];
}

template <class T>
void mergeSortDescend(std::vector<T>& data, unsigned int low, unsigned int high) {

    if (low >= high)
        return;
    if ((high - low) == 1)
    {
        if (data[high] > data[low])
        {
            T temp = data[high];
            data[high] = data[low];
            data[low] = temp;
        }
        return;
    }
    unsigned int mid = computeMean(low, high);
    mergeSortDescend<T>(data, low, mid);
    mergeSortDescend<T>(data, mid + 1, high);
    mergeDescend<T>(data, low, high, mid);
}

template <class T>
void mergeDescend (T* data, unsigned int low, unsigned int high, unsigned int mid)
{
    unsigned int i = low;
    unsigned int j = mid+1;
    unsigned int k = 0;
    
    std::vector<T> temp(high-low+1);
    while(i<=mid && j<=high)
    {
        if (data[i] >= data[j])
            temp[k++] = data[i++];
        else
            temp[k++] = data[j++];
    }
    while(i<=mid)
        temp[k++] = data[i++];
    while(j<=high)
        temp[k++] = data[j++];
    for (i=low; i<=high; i++)
        data[i] = temp[i-low];
}

template <class T>
void mergeSortDescend(T* data, unsigned int low, unsigned int high) {

    if (low >= high)
        return;
    if ((high - low) == 1)
    {
        if (data[high] > data[low])
        {
            T temp = data[high];
            data[high] = data[low];
            data[low] = temp;
        }
        return;
    }
    unsigned int mid = computeMean(low, high);
    mergeSortDescend<T>(data, low, mid);
    mergeSortDescend<T>(data, mid + 1, high);
    mergeDescend<T>(data, low, high, mid);
}