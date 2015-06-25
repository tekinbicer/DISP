#include <limits.h>
#include <iostream>
#include <ctime>
#include "data_region_2d_bare_base.h"
#include "gtest/gtest.h"

using namespace std;

const int kRowCount = 32;
const int kColCount = 1024*1024;
const int kRepetition = 10;

void Setup2DArray(DataRegion2DBareBase<float> &region2d_obj){
  for(int i=0; i<kRowCount; i++)
    for(int j=0; j<kColCount; j++)
      region2d_obj[i][j] = (i*kColCount)+j;
}

TEST(DataRegion2DTest, Create){
  clock_t begin = clock();
  /* Initiate local 2d array */
  float **array2d = new float* [kRowCount];
  for(int i=0; i<kRowCount; i++){
    array2d[i] = new float[kColCount];
  }
  clock_t orig_alloc_clk = clock() - begin;

  begin = clock();
  unique_ptr<DataRegion2DBareBase<float>> region_p(new DataRegion2DBareBase<float>(kRowCount, kColCount));
  clock_t my_alloc_clk = clock() - begin; 
  auto &region2d = *region_p;

  ASSERT_EQ(region2d.size(), kRowCount*kColCount*sizeof(float)) <<
    "Size of 2d region data and original differ";

  for(int i=0; i<kRowCount; i++){
    delete [] array2d[i];
  }
  delete [] array2d;

  std::cout << "Orig. 2D array allocation time: " << orig_alloc_clk << std::endl;
  std::cout << "2D data region allocation time: " << my_alloc_clk << std::endl;
}

TEST(DataRegion2DTest, AccessorsMutators){
  unique_ptr<DataRegion2DBareBase<float>> region_p(new DataRegion2DBareBase<float>(kRowCount, kColCount));
  auto &region2d_obj = *region_p;
  Setup2DArray(region2d_obj);

  ASSERT_EQ(region2d_obj.num_rows(), kRowCount);
  ASSERT_EQ(region2d_obj.num_cols(), kColCount);
  ASSERT_EQ(region2d_obj.count(), kRowCount*kColCount);

  ASSERT_EQ(region2d_obj.item(0, kColCount-1), kColCount-1);
  ASSERT_EQ(region2d_obj.item(0, 0), 0);

  float val = 13.;
  region2d_obj.item(0, 0, val);
  ASSERT_EQ(region2d_obj[0][0], val);
  val = 14.;
  region2d_obj.item(1, 1, val);
  ASSERT_EQ(region2d_obj[1][1], val);
}

TEST(DataRegion2DTest, Destructor){
  unique_ptr<DataRegion2DBareBase<float>> region_p(new DataRegion2DBareBase<float>(kRowCount, kColCount));
  auto &region2d_obj = *region_p;
  Setup2DArray(region2d_obj);
  
  DataRegion2DBareBase<float> copy_reg(kRowCount, kColCount);
}

TEST(DataRegion2DTest, Copy){
  unique_ptr<DataRegion2DBareBase<float>> region_p(new DataRegion2DBareBase<float>(kRowCount, kColCount));
  auto &region2d_obj = *region_p;
  Setup2DArray(region2d_obj);

  auto dr = new DataRegion2DBareBase<float>(kRowCount-1, kColCount);
  ASSERT_THROW(region2d_obj.copy(*dr), std::out_of_range);
  delete dr;
  dr = new DataRegion2DBareBase<float>(kRowCount, kColCount-1);
  ASSERT_THROW(region2d_obj.copy(*dr), std::out_of_range);
  delete dr;
  dr = new DataRegion2DBareBase<float>(kRowCount-1, kColCount-1);
  ASSERT_THROW(region2d_obj.copy(*dr), std::out_of_range);
  delete dr;

  /* Copy function */
  DataRegion2DBareBase<float> copy_reg(kRowCount, kColCount);
  region2d_obj.copy(copy_reg);

  for(size_t i=0; i<region2d_obj.num_rows(); i++)
    for(size_t j=0; j<region2d_obj.num_cols(); j++)
      ASSERT_EQ(region2d_obj[i][j], copy_reg[i][j]);


  /* Copy assignment */
  for(size_t i=0; i<region2d_obj.num_rows(); i++)
    for(size_t j=0; j<region2d_obj.num_cols(); j++){
      region2d_obj[i][j] = 0.5;
      ASSERT_NE(region2d_obj[i][j], copy_reg[i][j]);
    }
  copy_reg = region2d_obj;
  for(size_t i=0; i<region2d_obj.num_rows(); i++)
    for(size_t j=0; j<region2d_obj.num_cols(); j++)
      ASSERT_EQ(region2d_obj[i][j], copy_reg[i][j]);

  /* Copy constructor */
  DataRegion2DBareBase<float> copy_reg1(region2d_obj);
  for(size_t i=0; i<region2d_obj.num_rows(); i++)
    for(size_t j=0; j<region2d_obj.num_cols(); j++)
      ASSERT_EQ(region2d_obj[i][j], copy_reg1[i][j]);
}

TEST(DataRegion2DTest, WriteReadTest){
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

  ASSERT_EQ(sum_o, sum_m);

  std::cout << "2D region avg. write time: " << 1.*my_write_clk/kRepetition << std::endl;
  std::cout << "2D region avg. read time: " << 1.*my_read_clk/kRepetition << std::endl;

  sum_m=0.0;
  begin = clock();
  for(int r=0; r<kRepetition; r++)
    for(size_t i=0; i<kRowCount; i++)
      for(size_t j=1; j<kColCount; j++)
        sum_m += region2d_obj.item(i, j) + region2d_obj.item(i, j-1);
  clock_t my_read_func_clk = clock() - begin;

  ASSERT_EQ(sum_m, sum_o);

  sum_m=0.0;
  for(int i=0; i<kRowCount; i++)
    for(int j=0; j<kColCount; j++)
      region2d_obj[i][j] = (i*kColCount)+j;
  for(size_t i=0; i<kRowCount; i++)
    for(size_t j=0; j<kColCount; j++)
      sum_m += region2d_obj[i][j];
  float tot = (1.*kRowCount*kColCount)*((kRowCount*kColCount)-1)/2.0;
  ASSERT_EQ(sum_m, tot);

  for(int i=0; i<kRowCount; i++)
    delete array2d[i];
  delete array2d;

  std::cout << "2D region avg. read time using func.: " << 1.*my_read_func_clk/kRepetition << std::endl;
}
