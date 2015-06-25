#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include "reduction_space_a.h"
#include "disp_engine_reduction.h"

using namespace std;

const int64_t kColCount = 1024*1024*1024;
const int64_t kCacheSize = 32*1024;//32*1024; //4*1024*1024;
const int kRepeat = 1;

class ReductionSpaceSum : public AReductionSpaceBase<ReductionSpaceSum, int64_t>{
  public:

    const int64_t kMyLocalCount = 5;
    int64_t *my_local = nullptr;

    ReductionSpaceSum() : AReductionSpaceBase(1, 1) {
      my_local = new int64_t[kMyLocalCount];
    };

    void Reduce(MirroredRegionBareBase<int64_t> &input){
      auto &reduction_objs = reduction_objects();

      for(int j=0; j<kRepeat; j++)
        for(int64_t i=0; i<input.count(); i++)
          reduction_objs[0][0] = reduction_objs[0][0] + input[i];
    };

    virtual void LocalSynchWith(ReductionSpaceSum &input_reduction_space) {
      std::chrono::milliseconds timespan(1000);
      std::this_thread::sleep_for(timespan);
      auto &ri = input_reduction_space.reduction_objects();
      auto &ro = (this->reduction_objects());

      if(ri.num_rows()!=ro.num_rows() || ri.num_cols()!=ro.num_cols())
        throw std::range_error("Local and destination reduction objects have different dimension sizes!");

      for(int i=0; i<ro.num_rows(); i++)
        for(int j=0; j<ro.num_cols(); j++)
          ro[i][j] += ri[i][j];
    };

    // Deep copy
    void CopyTo(ReductionSpaceSum &target){
      copy(my_local, my_local+kMyLocalCount, target.my_local);
    };

    ~ReductionSpaceSum(){
      delete my_local;
    };
};

class DISPEngineTest : public testing::Test {
  protected:
    static ReductionSpaceSum *reduction_space;

    static ADataRegion<int64_t> *input;

    /// Before running the test case
    static void SetUpTestCase(){
      reduction_space = new ReductionSpaceSum();
      auto &reduction_objs = reduction_space->reduction_objects();
      int64_t init_val = 0;
      reduction_objs.SetAllItems(init_val);

      input = new DataRegionBareBase<int64_t>(kColCount);
      uint64_t total=0;

      for(int64_t i=0; i<kColCount; i++){
        (*input)[i] = i;
        total+=i;
      }

      cout << "Total orig=" << total << endl;
    };

    static void TearDownTestCase(){
      delete reduction_space;
      reduction_space = nullptr;
      delete input;
      input = nullptr;
    };

    /// Before each test calls constructor and destructor
    DISPEngineTest(){
      auto &reduction_objs = reduction_space->reduction_objects();
      int64_t init_val = 0;
      reduction_objs.SetAllItems(init_val);

      for(int64_t i=0; i<reduction_space->kMyLocalCount; i++){
        reduction_space->my_local[i] = reduction_space->kMyLocalCount-i;
      } 
    }; 
    
    ~DISPEngineTest(){ };
};

ReductionSpaceSum* DISPEngineTest::reduction_space = nullptr;
ADataRegion<int64_t>* DISPEngineTest::input = nullptr;

TEST_F(DISPEngineTest, ReduceFunc){
  DISPEngineBase<ReductionSpaceSum , int64_t> *engine = 
    new DISPEngineReduction<ReductionSpaceSum , int64_t>(
      nullptr, // DISPCommBase
      reduction_space,  // ReductionSpaceBase<RST, DT> *conf_reduction_space_i, 
      16                 // int num_reduction_threads_ (if 0, then auto set)
      );

  int64_t req_number = kCacheSize/sizeof(int64_t);

  engine->RunParallelReduction(*input, req_number);

  engine->Print();

  //engine->SeqInPlaceLocalSynchWrapper();
  /*
  cout << "Total after local synch: " << 
    reduction_space->reduction_objects()[0][0] << endl;
  */

  engine->ParInPlaceLocalSynchWrapper();
  cout << "Total after parallel local synch: " << 
    reduction_space->reduction_objects()[0][0] << endl;

  delete engine;
}

