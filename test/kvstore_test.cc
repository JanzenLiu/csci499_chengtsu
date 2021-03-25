#include "kvstore/kvstore.h"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

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

// A test fixture for testing of the KVStore persistence feature.
// It handles something related to the temporary file to use.
class PersistenceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Prepare a temporary filename to use.
    filename_ = fs::temp_directory_path() / "kvstore_test.data";
    // Delete the file in the case it already exists.
    remove(filename_.c_str());
  }

  void TearDown() override {
    // Delete the temporary file.
    remove(filename_.c_str());
  }

  // Returns the current size of the temporary file.
  int GetFileSize() {
    return fs::file_size(fs::path{filename_});
  }

  string filename_;
};

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

// Tests the basic functionality to load from and save to file.
TEST_F(PersistenceTest, PersistenceTest) {
  {
    // Load from a non-existent file.
    KVStore store(filename_);
    ASSERT_TRUE(store.Empty());
    // Store all kinds of operations to the file.
    store.Put("k1", "v1");
    store.Put("k1", "v2");
    store.Put("k2", "v3");
    store.Clear();
    store.Put("k3", "v4");
    store.Put("k3", "v5");
    store.Put("k4", "v6");
    store.Put("k5", "v7");
    store.Remove("k4");
  }
  {
    // Load from an existing file who has experienced one run.
    KVStore store(filename_);
    ASSERT_EQ(2, store.Size());
    EXPECT_TRUE(VectorEq({"v4", "v5"}, store.Get("k3")));
    EXPECT_TRUE(VectorEq({"v7"}, store.Get("k5")));
    // Store some operations to the file.
    store.Put("k5", "v8");
    store.Put("k6", "v9");
    store.Remove("k3");
  }
  {
    // Load from a file who has experienced more than one run.
    KVStore store(filename_);
    ASSERT_EQ(2, store.Size());
    EXPECT_TRUE(VectorEq({"v7", "v8"}, store.Get("k5")));
    EXPECT_TRUE(VectorEq({"v9"}, store.Get("k6")));
  }
}

// Tests the functionality to deal with corrupted file.
TEST_F(PersistenceTest, CorruptedFileTest) {
  {
    KVStore store(filename_);
    store.Put("k1", "v1");
    store.Put("k2", "v2");
  }
  int old_size = GetFileSize();
  {
    KVStore store(filename_);
    store.Put("k1", "v3");
  }
  {
    // Simulate a corruption by deleting the last byte from the file.
    int new_size = GetFileSize();
    truncate(filename_.c_str(), new_size-1);
    // Check that the last operation, which is corrupted,
    // is not loaded into the KVStore.
    KVStore store(filename_);
    ASSERT_EQ(2, store.Size());
    EXPECT_TRUE(VectorEq({"v1"}, store.Get("k1")));
    EXPECT_TRUE(VectorEq({"v2"}, store.Get("k2")));
  }
  // Check that the last operation is removed from the file.
  ASSERT_EQ(old_size, GetFileSize());
}

// Tests whether the persistence works well with long keys and values.
TEST_F(PersistenceTest, LongStringTest) {
  vector<int> lens = {100, 1000, 10000, 100000};
  {
    // Store some operations to the file.
    KVStore store(filename_);
    for (int len : lens) {
      string key = string(len, 'k');
      string value = string(len, 'v');
      store.Put(key, value);
      store.Put(key, value);
      store.Put(key, value);
    }
  }
  {
    KVStore store(filename_);
    ASSERT_EQ(4, store.Size());
    for (int len : lens) {
      string key = string(len, 'k');
      string value = string(len, 'v');
      EXPECT_TRUE(VectorEq({value, value, value}, store.Get(key)));
    }
  }
}

// Tests whether the persistence works well with empty keys and values.
TEST_F(PersistenceTest, EmptyStringTest) {
  {
    // Stores some operations to the file.
    KVStore store(filename_);
    store.Put("k1", "");
    store.Put("", "v1");
    store.Put("", "");
  }
  {
    KVStore store(filename_);
    ASSERT_EQ(2, store.Size());
    EXPECT_TRUE(VectorEq({""}, store.Get("k1")));
    EXPECT_TRUE(VectorEq({"v1", ""}, store.Get("")));
  }
}

// Tests whether the persistence works well with keys and values
// containing non-alphanumeric characters.
TEST_F(PersistenceTest, NonAlphanumCharTest) {
  string str1 = "!@#$%";
  string str2 = "^&*()";
  string str3 = "-=_+~";
  string str4 = "`{[]}";
  string str5 = "\\|:;'";
  string str6 = "\"<>,.";
  string str7 = "?/ \t\n";
  {
    // Stores some operations to the file.
    KVStore store(filename_);
    store.Put(str1, str2);
    store.Put(str1, str3);
    store.Put(str1, str4);
    store.Put(str5, str6);
    store.Put(str5, str7);
  }
  {
    KVStore store(filename_);
    ASSERT_EQ(2, store.Size());
    EXPECT_TRUE(VectorEq({str2, str3, str4}, store.Get(str1)));
    EXPECT_TRUE(VectorEq({str6, str7}, store.Get(str5)));
  }
}

int main(int argc, char **argv) {
  // Use a self-defined main function here to call InitGoogleLogging(),
  // otherwise, all glog messages (including INFO) will be directed to
  // STDERR and hence displayed to the terminal.
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  return RUN_ALL_TESTS();
}
