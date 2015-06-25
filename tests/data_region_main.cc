#include <limits.h>
#include <iostream>
#include <ctime>
#include <cfenv>
#include <cmath>
#include <memory>
#include <string>
#include "data_region_a.h"
#include "data_region_base.h"
#include "data_region_bare_base.h"

using namespace std;

const int kColCount = 16*16;
const int kRepetition = 10;
const int kReqUnits = 32;

void Create(){
  unique_ptr<ADataRegion<int>> region_p(new DataRegionBareBase<int>(kColCount));
  auto &region = *region_p;

}

void CM(){
  DataRegionBareBase<int> region(kColCount);

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  /* Copy assignment */
  DataRegionBareBase<int> replica_0(kColCount*2);

  for(size_t i=0; i<kColCount*2; ++i)
    replica_0[i]= i*2;

  replica_0 = region;
  for(size_t i=0; i<kColCount; ++i)
    replica_0[i]= replica_0[i]*2;
  for(size_t i=0; i<kColCount; ++i)

  /* Move assignment */
  DataRegionBareBase<int> replica_1 = move(replica_0);

}

void MNextMirroredRegion(){
  ADataRegion<int> *region_p = new DataRegionBareBase<int>(kColCount);
  auto &region = *region_p;

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  vector<MirroredRegionBareBase<int> *> mirrored_regions;

  auto mr = region.NextMirroredRegion(kReqUnits);
  int counter = 0;
  while(mr != nullptr){
    for(size_t i=0; i<mr->count(); i++)
      counter++ ;
    mirrored_regions.push_back(mr);
    mr = region.NextMirroredRegion(kReqUnits);
  }

  fesetround(FE_UPWARD);
  int mregion_count = rint(1.*region.num_cols()/kReqUnits);

  region.ResetMirroredRegionIter();

  auto mr2 = region.NextMirroredRegion(kReqUnits);
  int counter2 = 0;
  while(mr2 != nullptr){
    for(size_t i=0; i<mr2->count(); i++)
      counter2++; 
    mirrored_regions.push_back(mr2);
    mr2 = region.NextMirroredRegion(kReqUnits);
  }

  delete region_p;

  /*
   * Below results in double delete! Because of internal
   * delete that happen with region_p
   *
  for(auto &mr : mirrored_regions)
    delete mr;
  */
}

typedef struct mm{
  int a=0;
  uint64_t b=1011;
} Metadata;

void TestMDataRegion(){
  Metadata metadata;
  metadata.a=1024;
  ADataRegion<int> *region_p = new DataRegionBase<int, Metadata>(kColCount, &metadata);
  auto &region = *region_p;
  auto &region_r = *(dynamic_cast<DataRegionBase<int, Metadata>*>(region_p));

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  vector<MirroredRegionBase<int, Metadata> *> mirrored_regions;

  auto mr = dynamic_cast<MirroredRegionBase<int, Metadata>*>
    (region.NextMirroredRegion(kReqUnits));

  int counter = 0;
  while(mr != nullptr){
    for(size_t i=0; i<mr->count(); i++)
      counter++; 

    mirrored_regions.push_back(mr);
    mr = dynamic_cast<MirroredRegionBase<int, Metadata>*>
      (region.NextMirroredRegion(kReqUnits));
  }

  fesetround(FE_UPWARD);
  int mregion_count = rint(1.*region.num_cols()/kReqUnits);

  region.ResetMirroredRegionIter();

  auto mr2 = dynamic_cast<MirroredRegionBase<int, Metadata>*>
    (region.NextMirroredRegion(kReqUnits));
  int counter2 = 0;
  while(mr2 != nullptr){
    for(size_t i=0; i<mr2->count(); i++)
      counter2++; 

    mirrored_regions.push_back(mr2);
    mr2 = dynamic_cast<MirroredRegionBase<int, Metadata>*>
    (region.NextMirroredRegion(kReqUnits));
  }

  delete region_p;
  /*
   * Below results in double delete! Because of internal
   * delete that happen with region_p
   *
     for(auto &reg : mirrored_regions)
     delete reg;
   */

}

void DirectAssign(){
  unique_ptr<ADataRegion<int>> region_p(new DataRegionBareBase<int>(kColCount));
  auto &region = *region_p;

  uint64_t total_2x_real = 0;
  for(size_t i=0; i<kColCount; ++i){
    region[i] = i;
    total_2x_real += 2*i;
  }

  uint64_t total_2x_calc = 0; 
  for(size_t i=0; i<kColCount; ++i){
    region[i] *= 2;
    total_2x_calc += region[i];
  }

}

int main(){
  Create();
  DirectAssign();
  TestMDataRegion();
  MNextMirroredRegion();
  CM();
}
