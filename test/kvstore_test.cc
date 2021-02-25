#include "kvstore/kvstore.h"

#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using std::string;
using std::thread;
using std::vector;

// Tests the value equality of two std::vector<T>.
template<typename T>
::testing::AssertionResult
VectorEq(vector<T>&& expected, vector<T>&& actual){
  if (expected.size() != actual.size()) {
    return ::testing::AssertionFailure()
      << "actual.size() (" << actual.size() << ") != "
      << "expected.size() (" << expected.size() << ")";
  }
  for (size_t i = 0; i < expected.size(); ++i){
    if (expected[i] != actual[i]){
      return ::testing::AssertionFailure()
        << "actual[" << i << "] (" << actual[i] << ") != "
        << "expected[" << i << "] (" << expected[i] << ")";
    }
  }
  return ::testing::AssertionSuccess();
}

// Tests the basic functionality of each interface.
TEST(MapTest, MapTest) {
  KVStore store;
  EXPECT_TRUE(store.Empty());
  store.Put("k1", "v1");
  store.Put("k1", "v2");
  store.Put("k2", "v3");
  store.Put("k3", "v4");
  store.Put("k3", "v5");
  store.Put("k3", "v6");
  EXPECT_FALSE(store.Empty());
  EXPECT_EQ(3, store.Size());
  EXPECT_TRUE(VectorEq({"v1", "v2"}, store.Get("k1")));
  EXPECT_TRUE(VectorEq({"v3"}, store.Get("k2")));
  EXPECT_TRUE(VectorEq({"v4", "v5", "v6"}, store.Get("k3")));
  EXPECT_TRUE(VectorEq({}, store.Get("k4")));
  store.Remove("k3");
  EXPECT_EQ(2, store.Size());
  EXPECT_TRUE(VectorEq({}, store.Get("k3")));
  store.Clear();
  EXPECT_TRUE(store.Empty());
}

// Tests the initializer list constructor.
TEST(MapTest, InitListConstructorTest) {
  KVStore store{ {"k1", {"v1"}}, {"k2", {"v2", "v3"}} };
  ASSERT_EQ(2, store.Size());
  EXPECT_TRUE(VectorEq({"v1"}, store.Get("k1")));
  EXPECT_TRUE(VectorEq({"v2", "v3"}, store.Get("k2")));
  store.Put("k1", "v4");
  store.Put("k2", "v5");
  EXPECT_TRUE(VectorEq({"v1", "v4"}, store.Get("k1")));
  EXPECT_TRUE(VectorEq({"v2", "v3", "v5"}, store.Get("k2")));
}

// Tests the correctness of the return value of `KVStore::Put()`.
TEST(ReturnValueTest, PutTest) {
  KVStore store;
  EXPECT_TRUE(store.Put("k1", "v1"));
  EXPECT_TRUE(store.Put("k2", "v2"));
  EXPECT_FALSE(store.Put("k1", "v3"));
  EXPECT_FALSE(store.Put("k1", "v4"));
  EXPECT_TRUE(store.Put("k3", "v5"));
  EXPECT_FALSE(store.Put("k2", "v6"));
}

// Tests the correctness of the return value of `KVStore:Remove()`.
TEST(ReturnValueTest, RemoveTest) {
  KVStore store;
  store.Put("k1", "v1");
  store.Put("k3", "v2");
  store.Put("k3", "v3");
  EXPECT_TRUE(store.Remove("k1"));
  EXPECT_FALSE(store.Remove("k2"));
  EXPECT_TRUE(store.Remove("k3"));
}

// Tests whether `KVStore::Get()` returns a copy, instead of a reference.
TEST(ReturnValueTest, GetReturnsCopyTest) {
  KVStore store;
  store.Put("k1", "v1");
  store.Get("k1").push_back("v2");
  EXPECT_TRUE(VectorEq({"v1"}, store.Get("k1")));
}

// Tests whether `KVStore::Get()` does not insert an empty vector
// for not existed keys, which `std::unordered_map::operator[]` does.
TEST(SideEffectTest, GetTest) {
  KVStore store;
  store.Get("k1");
  EXPECT_TRUE(store.Empty());
}

// Tests the thread-safety of concurrent writes.
TEST(ConcurrencyTest, ConcurrentWriteTest) {
  KVStore store;
  size_t num_threads = 4;
  size_t num_reps_per_thread = 100;
  vector<thread> threads;
  for (size_t tid = 0; tid < num_threads; ++tid) {
    thread t([&store, &num_reps_per_thread](){
      for (int rep = 0; rep < num_reps_per_thread; ++rep) {
        store.Put("key", "val");
      }
    });
    threads.push_back(std::move(t));
  }
  for (size_t tid = 0; tid < num_threads; ++tid) {
    threads[tid].join();
  }
  size_t num_values = num_threads*num_reps_per_thread;
  EXPECT_TRUE(VectorEq(vector<string>(num_values, "val"), store.Get("key")));
}

// Tests the thread-safety of concurrent reads and writes.
TEST(ConcurrencyTest, ConcurrentReadWriteTest) {
  // If the implementation is thread-unsafe, it's possible that one write
  // thread erases an iterator right after another read thread just got it,
  // making the read thread's iterator invalid.
  // To maximize the chance of it to happen (if it will), we test it mainly
  // via intensive concurrent `Get()` and `Clear()`.
  KVStore store;
  size_t num_keys = 2;
  size_t num_read_threads = 2;
  vector<thread> read_threads;
  for (size_t tid = 0; tid < num_read_threads; ++tid) {
    thread t([&](){
      for (size_t rep = 0; rep < 100; ++rep) {
        int k = rand() % num_keys;
        auto values = store.Get("k" + std::to_string(k));
        for (auto value : values) {
          EXPECT_EQ("val", value);
        }
      }
    });
    read_threads.push_back(std::move(t));
  }

  thread put_thread([&store, &num_keys](){
    for (size_t rep = 0; rep < 100; ++rep) {
      for (int k = 0; k < num_keys; ++k) {
        store.Put("k" + std::to_string(k), "val");
      }
    }
  });

  thread clear_thread([&store](){
    for (size_t rep = 0; rep < 100; ++rep) {
      store.Clear();
    }
  });

  for (size_t tid = 0; tid < num_read_threads; ++tid) {
    read_threads[tid].join();
  }
  put_thread.join();
  clear_thread.join();
}
