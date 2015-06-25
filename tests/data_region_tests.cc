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
#include "gtest/gtest.h"

using namespace std;

const int kColCount = 16*1024*1024;
const int kRepetition = 10;
const int kReqUnits = 32;

class DataRegionTest : public ::testing::Test{
};

TEST(DataRegionTest, Create){
  unique_ptr<ADataRegion<int>> region_p(new DataRegionBareBase<int>(kColCount));
  auto &region = *region_p;

  ASSERT_EQ(region.size(), kColCount*sizeof(int)) << 
    "Size of region data and original differ";
}

TEST(DataRegionTest, DirectAssign){
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

  ASSERT_EQ(total_2x_real, total_2x_calc) <<
    "Direct: Totals are not equal";
}

TEST(DataRegionTest, IndirectAssign){
  unique_ptr<ADataRegion<int>> region_p(new DataRegionBareBase<int>(kColCount));
  auto &region = *region_p;

  uint64_t total_2x_real = 0;
  for(size_t i=0; i<kColCount; ++i){
    region.item(i, i);
    total_2x_real += 2*i;
  }

  uint64_t total_2x_calc = 0; 
  for(size_t i=0; i<kColCount; ++i){
    region.item(i, region.item(i)*2);
    total_2x_calc += region.item(i);
  }

  ASSERT_EQ(total_2x_real, total_2x_calc) <<
    "Indirect: Totals are not equal";
}

/* Tests for Rule-of-Five: Copy/Move/Destructor */
TEST(DataRegionTest, Constructors){
  DataRegionBareBase<int> region(kColCount);

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  /* Copy constructor */
  DataRegionBareBase<int> replica_0(region);
  for(size_t i=0; i<kColCount; ++i)
    replica_0[i]= replica_0[i]*2;
  for(size_t i=0; i<kColCount; ++i)
    ASSERT_EQ(replica_0[i], region[i]*2) <<
      "Copy constructor, (), didn't work";

  DataRegionBareBase<int> replica_1 = region;
  for(size_t i=0; i<kColCount; ++i)
    replica_1[i]= replica_1[i]*4;
  for(size_t i=0; i<kColCount; ++i)
    ASSERT_EQ(replica_1[i], region[i]*4) <<
      "Copy constructor, =, didn't work";

  /* Move constructor */
  DataRegionBareBase<int> replica_2(move(replica_0));
  ASSERT_EQ(replica_2.size(), kColCount*sizeof(int)) <<
    "Sizes are not equal after move constructor";
  ASSERT_EQ(replica_0.size(), 0) <<
    "Source instance size is larger than 0 after move constructor";
  ASSERT_EQ(&replica_0[0], nullptr) <<
    "Source instance data pointer is not nullptr after move constructor";
  for(size_t i=0; i<kColCount; ++i)
    ASSERT_EQ(replica_2[i], region[i]*2) <<
      "Values do not match after move constructor";
}

TEST(DataRegionTest, Destructor){
  DataRegionBareBase<int> region(kColCount);
}

TEST(DataRegionTest, Assignments){
  DataRegionBareBase<int> region(kColCount);

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  /* Copy assignment */
  DataRegionBareBase<int> replica_0(kColCount*2);
  ASSERT_EQ(replica_0.size(), kColCount*2*sizeof(int));
  for(size_t i=0; i<kColCount*2; ++i)
    replica_0[i]= i*2;

  replica_0 = region;
  ASSERT_EQ(replica_0.size(), kColCount*sizeof(int));
  for(size_t i=0; i<kColCount; ++i)
    replica_0[i]= replica_0[i]*2;
  for(size_t i=0; i<kColCount; ++i)
    ASSERT_EQ(replica_0[i], region[i]*2) <<
      "Values do not match after copy constructor";

  /* Move assignment */
  DataRegionBareBase<int> replica_1 = move(replica_0);
  ASSERT_EQ(replica_1.size(), kColCount*sizeof(int)) <<
    "Sizes are not equal after move assignment";
  ASSERT_EQ(replica_0.size(), 0) <<
    "Source instance size is larger than 0 after move assignment";
  ASSERT_EQ(&replica_0[0], nullptr) <<
    "Source instance data pointer is not nullptr after move assingment";
  for(size_t i=0; i<kColCount; ++i)
    ASSERT_EQ(replica_1[i], region[i]*2) <<
      "Values do not match after move constructor";
}

TEST(DataRegionTest, NextMirroredRegion){
  ADataRegion<int> *region_p = new DataRegionBareBase<int>(kColCount);
  auto &region = *region_p;

  for(size_t i=0; i<kColCount; ++i)
    region[i]= i;

  vector<MirroredRegionBareBase<int> *> mirrored_regions;

  auto mr = region.NextMirroredRegion(kReqUnits);
  int counter = 0;
  while(mr != nullptr){
    for(size_t i=0; i<mr->count(); i++)
      ASSERT_EQ(counter++, (*mr)[i]) << 
        "Mirrored region values do not match";
    mirrored_regions.push_back(mr);
    mr = region.NextMirroredRegion(kReqUnits);
  }
  ASSERT_EQ(region[region.cols()-1], counter-1) << 
    "Mirrored region counter did not match with region columns";

  fesetround(FE_UPWARD);
  int mregion_count = rint(1.*region.num_cols()/kReqUnits);
  ASSERT_EQ(mregion_count, mirrored_regions.size()) << 
    "Mirrored region counts didn't match";

  region.ResetMirroredRegionIter();

  auto mr2 = region.NextMirroredRegion(kReqUnits);
  int counter2 = 0;
  while(mr2 != nullptr){
    for(size_t i=0; i<mr2->count(); i++)
      ASSERT_EQ(counter2++, (*mr2)[i]) << 
        "Mirrored region values didn't match";
    mirrored_regions.push_back(mr2);
    mr2 = region.NextMirroredRegion(kReqUnits);
  }

  delete region_p;
}

typedef struct mm{
  int a=0;
  uint64_t b=1011;
} Metadata;

TEST(DataRegionTest, DataRegion){
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
  ASSERT_NE(mr, nullptr);

  int counter = 0;
  while(mr != nullptr){
    for(size_t i=0; i<mr->count(); i++)
      ASSERT_EQ(counter++, (*mr)[i]) << 
        "Base: Mirrored region values do not match";
    ASSERT_EQ(&(region_r.metadata()), &(mr->metadata()));
    ASSERT_EQ(region_r.metadata().a, mr->metadata().a);
    ASSERT_EQ(region_r.metadata().b, mr->metadata().b);

    mirrored_regions.push_back(mr);
    mr = dynamic_cast<MirroredRegionBase<int, Metadata>*>
      (region.NextMirroredRegion(kReqUnits));
  }
  ASSERT_EQ(region[region.count()-1], counter-1) << 
    "Base: Mirrored region counter did not match with region columns";

  fesetround(FE_UPWARD);
  int mregion_count = rint(1.*region.num_cols()/kReqUnits);
  ASSERT_EQ(mregion_count, mirrored_regions.size()) << 
    "Base: Mirrored region counts didn't match";

  region.ResetMirroredRegionIter();

  auto mr2 = dynamic_cast<MirroredRegionBase<int, Metadata>*>
    (region.NextMirroredRegion(kReqUnits));
  int counter2 = 0;
  while(mr2 != nullptr){
    for(size_t i=0; i<mr2->count(); i++)
      ASSERT_EQ(counter2++, (*mr2)[i]) << 
        "Base: Mirrored region values didn't match";

    ASSERT_EQ(&(region_r.metadata()), &(mr2->metadata()));
    ASSERT_EQ(region_r.metadata().a, mr2->metadata().a);
    ASSERT_EQ(region_r.metadata().b, mr2->metadata().b);

    mirrored_regions.push_back(mr2);
    mr2 = dynamic_cast<MirroredRegionBase<int, Metadata>*>
    (region.NextMirroredRegion(kReqUnits));
  }

  delete region_p;
}
