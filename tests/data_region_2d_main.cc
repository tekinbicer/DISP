#include <limits.h>
#include <iostream>
#include <ctime>
#include "data_region_2d_bare_base.h"

using namespace std;

const int kRowCount = 2;
const int kColCount = 1024*1024;
const int kRepetition = 1;

void Setup2DArray(DataRegion2DBareBase<float> &region2d_obj){
  for(int i=0; i<kRowCount; i++)
    for(int j=0; j<kColCount; j++)
      region2d_obj[i][j] = (i*kColCount)+j;
}

int main(){
  unique_ptr<DataRegion2DBareBase<float>> region_p(new DataRegion2DBareBase<float>(kRowCount, kColCount));
  auto &region2d_obj = *region_p;
  Setup2DArray(region2d_obj);

  /* Initiate local 2d array */
  float **array2d = new float* [kRowCount];
  for(int i=0; i<kRowCount; i++)
    array2d[i] = new float[kColCount];
  clock_t begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(int i=0; i<kRowCount; i++)
      for(int j=0; j<kColCount; j++)
        array2d[i][j] = 2*i+j;
  clock_t orig_write_clk = clock() - begin;

  float sum_o =0.0;
  begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(int i=0; i<kRowCount; i++)
      for(int j=1; j<kColCount; j++)
        sum_o += array2d[i][j]+array2d[i][j-1];
  clock_t orig_read_clk = clock() - begin;
  std::cout << "2D array avg. write time: " << 1.*orig_write_clk/kRepetition << std::endl;
  std::cout << "2D array avg. read time: " << 1.*orig_read_clk/kRepetition << std::endl;

  float sum_m =0.0;
  begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(size_t i=0; i<kRowCount; i++)
      for(size_t j=0; j<kColCount; j++)
        region2d_obj[i][j] = 2*i+j;
  clock_t my_write_clk = clock() - begin;

  begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(size_t i=0; i<kRowCount; i++)
      for(size_t j=1; j<kColCount; j++)
        sum_m += region2d_obj[i][j]+region2d_obj[i][j-1];
  clock_t my_read_clk = clock() - begin;


  std::cout << "2D region avg. write time: " << 1.*my_write_clk/kRepetition << std::endl;
  std::cout << "2D region avg. read time: " << 1.*my_read_clk/kRepetition << std::endl;

  sum_m=0.0;
  begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(size_t i=0; i<kRowCount; i++)
      for(size_t j=1; j<kColCount; j++)
        sum_m += region2d_obj.item(i, j) + region2d_obj.item(i, j-1);
  clock_t my_read_func_clk = clock() - begin;


  sum_m=0.0;
  for(int i=0; i<kRowCount; i++)
    for(int j=0; j<kColCount; j++)
      region2d_obj[i][j] = (i*kColCount)+j;
  for(size_t i=0; i<kRowCount; i++)
    for(size_t j=0; j<kColCount; j++)
      sum_m += region2d_obj[i][j];
  float tot = (1.*kRowCount*kColCount)*((kRowCount*kColCount)-1)/2.0;

  std::cout << "2D region avg. read time using func.: " << 1.*my_read_func_clk/kRepetition << std::endl;

  for(int i=0; i<kRowCount; i++)
    delete[] array2d[i];
  delete[]  array2d;

}
