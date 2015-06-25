#include "reduction_space_a.h"
#include "gtest/gtest.h"

using namespace std;


class ReductionSpaceSum : public AReductionSpaceBase<ReductionSpaceSum, float>{
  public:

    const int kMyLocalCount = 5;
    int *my_local = nullptr;

    ReductionSpaceSum() : AReductionSpaceBase(1, 1) {
      my_local = new int[kMyLocalCount];
    };

    void Reduce(MirroredRegionBareBase<float> &input){
      auto &reduction_objs = reduction_objects();

      for(int i=0; i<input.count(); i++)
        reduction_objs[0][0] = reduction_objs[0][0] + input[i];
    };

    void CopyTo(ReductionSpaceSum &target){
      copy(my_local, my_local+kMyLocalCount, target.my_local);
    };

    ~ReductionSpaceSum(){
      delete my_local;
    };
};

const int kColCount = 32*1024*1024;
const int kReqUnits = 10;

///TODO: Fix below

class ReductionSpaceTest : public testing::Test {
  protected:
    static ReductionSpaceSum *reduction_space;

    static ADataRegion<float> *input;

    /// Before running the test case
    static void SetUpTestCase(){
      reduction_space = new ReductionSpaceSum();
      input = new DataRegionBareBase<float>(kColCount);

      for(int i=0; i<kColCount; i++){
        (*input)[i] = i;
      }
    };

    static void TearDownTestCase(){
      delete reduction_space;
      reduction_space = nullptr;
      delete input;
      input = nullptr;
    };

    /// Before each test calls constructor and destructor
    ReductionSpaceTest(){
      auto &reduction_objs = reduction_space->reduction_objects();
      float init_val = 0.;
      reduction_objs.SetAllItems(init_val);

      for(int i=0; i<reduction_space->kMyLocalCount; i++){
        reduction_space->my_local[i] = reduction_space->kMyLocalCount-i;
      }
    };
    
    ~ReductionSpaceTest(){
    };
};

ReductionSpaceSum* ReductionSpaceTest::reduction_space = nullptr;
ADataRegion<float>* ReductionSpaceTest::input = nullptr;

TEST_F(ReductionSpaceTest, ReduceFunc){
  auto &rspace = *reduction_space;

  auto mr = input->NextMirroredRegion(input->count());
  rspace.Reduce(*mr);

  float sum_a = 1.*kColCount*(kColCount-1)/2.;
  EXPECT_EQ(rspace[0][0], sum_a);

  input->ResetMirroredRegionIter();
}

TEST_F(ReductionSpaceTest, CloneFunc){
  auto cbspace = 
    static_cast<AReductionSpaceBase<ReductionSpaceSum, float>*>(reduction_space);

  AReductionSpaceBase<ReductionSpaceSum, float>* cbbspace = (*cbspace).Clone();

  auto cspace = 
    static_cast<ReductionSpaceSum*>(cbbspace);

  auto mr = input->NextMirroredRegion(input->count());
  cspace->Reduce(*mr);

  float sum_a = 1.*kColCount*(kColCount-1)/2.;
  EXPECT_EQ((*cspace)[0][0], sum_a);

  for(int i=0; i<cspace->kMyLocalCount; i++){
    EXPECT_EQ(cspace->my_local[i], reduction_space->my_local[i]);
    cspace->my_local[i] = (cspace->kMyLocalCount)-reduction_space->my_local[i];
  }

  for(int i=0; i<cspace->kMyLocalCount; i++){
    EXPECT_EQ(cspace->my_local[i], 
        (cspace->kMyLocalCount) - reduction_space->my_local[i]);
  }

  input->ResetMirroredRegionIter();
  delete cspace;
}
