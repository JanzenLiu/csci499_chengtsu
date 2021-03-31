#include "kvstore/kvstore.h"

#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <glog/logging.h>

using std::ifstream;
using std::initializer_list;
using std::ofstream;
using std::pair;
using std::string;
using std::vector;

// Change types that will be persisted to file.
enum ChangeType : char { kPut, kRemove, kClear };

KVStore::KVStore() : map_(), mutex_(), log_(), filename_() {}

KVStore::KVStore(initializer_list<pair<string, vector<string>>> args)
    : map_(), mutex_(), log_(), filename_() {
  for (const auto& p : args) {
    map_[p.first] = p.second;
  }
}

KVStore::KVStore(const string& filename)
    : map_(), mutex_(), log_(ofstream()), filename_(filename) {
  // Open the file in read mode to load changes.
  ifstream infile(filename, ifstream::binary);
  if (infile) {
    LOG(INFO) << "Successfully opened file " << filename << " in read mode.";
    int cur_pos = 0;
    int num_records = 0;
    bool corrupted = false;
    // Keep Loading changes until EOF reached naturally or corruption found.
    while (infile.peek() != EOF && !corrupted) {
      // Get the position of the current character in the input stream,
      // which will be used as the starting position to truncate the file
      // if a corruption is found.
      cur_pos = infile.tellg();
      // Consider a failure to load change is cause by corrupted data.
      corrupted = !LoadChange(infile);
      if (!corrupted) {
        ++num_records;
      }
    }
    LOG(INFO) << num_records << " records loaded.";
    infile.close();
    if (corrupted) {
      LOG(ERROR) << "Found corruption starting from position " << cur_pos;
      // Delete all content starting from position `cur_pos` from the file.
      TruncateTrailingContent(cur_pos);
    } else {
      // Open the file. This is necessary because the member initializer list
      // only created an empty ofstream, just to make sure the optional has
      // a value.
      ReopenFile();
    }
  } else {
    LOG(INFO) << "File " << filename << " does not exists, creating it...";
    // Create the file and open it.
    ReopenFile();
  }
}

void KVStore::TruncateTrailingContent(int start_pos) {
  // Close the file stream if it is open.
  if (log_->is_open()) {
    log_->close();
  }
  // Delete all content starting from position `start_pos` from the file.
  if (truncate(filename_.c_str(), start_pos) != 0) {
    LOG(FATAL) << "Failed to truncate trailing content from position "
               << start_pos;
  } else {
    LOG(INFO) << "Successfully truncated trailing content from position "
              << start_pos;
  }
  // Reopen the file stream.
  ReopenFile();
}

void KVStore::ReopenFile() {
  // Close the file stream if it is open.
  if (log_->is_open()) {
    log_->close();
  }
  // Reopen the file stream.
  log_->open(filename_, ofstream::app | ofstream::binary);
  if (!log_->is_open()) {
    LOG(FATAL) << "Failed to reopen file " << filename_ << " in write mode.";
  }
  LOG(INFO) << "Successfully reopened file " << filename_ << " in write mode.";
}

vector<string> KVStore::Get(const string& key) const {
  // A read-write lock is needed here to avoid deleted
  // or changed iterator.
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = map_.find(key);
  if (iter != map_.end()) {
    return iter->second;
  }
  return {};
}

bool KVStore::Put(const string& key, const string& value) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  map_[key].push_back(value);
  // Persist the put operation to the associated file if applicable.
  if (log_.has_value()) {
    char c = ChangeType::kPut;
    // Get the position of the current character in the output stream.
    int cur_pos = log_->tellp();
    if (!log_->write(&c, sizeof c) ||
        !DumpString(key) || !DumpString(value) || !log_->flush()) {
      LOG(ERROR) << "Failed to persist operation Put("
                 << key << ", " << value << ") to file.";
      // Delete all content starting from position `cur_pos` from the file.
      TruncateTrailingContent(cur_pos);
      return false;
    }
  }
  LOG(INFO) << "Successfully Put(" << key << ", " << value << ") to kvstore.";
  return true;
}

bool KVStore::Remove(const string& key, bool& key_existed) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  key_existed = map_.erase(key);
  // Persist the remove operation to the associated file if applicable.
  if (log_.has_value()) {
    char c = ChangeType::kRemove;
    // Get the position of the current character in the output stream.
    int cur_pos = log_->tellp();
    if (!log_->write(&c, sizeof c) || !DumpString(key) || !log_->flush()) {
      LOG(ERROR) << "Failed to persist operation Remove("
                 << key << ") to file.";
      // Delete all content starting from position `cur_pos` from the file.
      TruncateTrailingContent(cur_pos);
      return false;
    }
  }
  LOG(INFO) << "Successfully Remove(" << key << ") from kvstore.";
  return key_existed;
}

bool KVStore::Remove(const string& key) {
  bool key_existed;
  return Remove(key, key_existed);
}

bool KVStore::Clear() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  map_.clear();
  // Persist the clear operation to the associated file if applicable.
  if (log_.has_value()) {
    char c = ChangeType::kClear;
    // Get the position of the current character in the output stream.
    int cur_pos = log_->tellp();
    if (!log_->write(&c, sizeof c) || !log_->flush()) {
      LOG(ERROR) << "Failed to persist operation Clear() to file.";
      // Delete all content starting from position `cur_pos` from the file.
      TruncateTrailingContent(cur_pos);
      return false;
    }
  }
  LOG(INFO) << "Successfully Clear() kvstore.";
  return true;
}

size_t KVStore::Size() const noexcept {
  return map_.size();
}

bool KVStore::Empty() const noexcept {
  return map_.empty();
}

void KVStore::Print() const {
  // A read-write lock is needed here to avoid deleted
  // or changed iterator.
  std::shared_lock<std::shared_mutex> lock(mutex_);
  for (auto iter = map_.begin(); iter != map_.end(); ++iter) {
    std::cout << iter->first << ": [ ";
    for (auto value : iter->second) {
      std::cout << value << " ";
    }
    std::cout << "]" << std::endl;
  }
}

bool KVStore::LoadChange(ifstream& infile) {
  // Get the type of the next change.
  char type;
  infile.get(type);
  if (!infile.good()) {
    return false;
  }
  switch (type) {
    case ChangeType::kPut: {
      string key, value;
      if (!LoadString(infile, key)) { return false; }
      if (!LoadString(infile, value)) { return false; }
      map_[key].push_back(value);
      break;
    }
    case ChangeType::kRemove: {
      string key;
      if (!LoadString(infile, key)) { return false; }
      map_.erase(key);
      break;
    }
    case ChangeType::kClear: {
      map_.clear();
      break;
    }
    default: {
      LOG(ERROR) << "Unknown change type loaded: " << int{type};
      return false;
    }
  }
  return true;
}

bool KVStore::LoadString(ifstream& infile, string& str) {
  // Decode the length of the string first.
  size_t len = 0;
  bool len_end = false;
  int n_shifts = 0;
  // Load bytes until a byte with the highest bit set to 1.
  while (!len_end) {
    // Load a byte.
    char b;
    infile.get(b);
    if (!infile.good()) {
      return false;
    }
    // Add the lowest 7 bits to the length.
    len |= ((b & 0x7F) << n_shifts);
    n_shifts += 7;
    // Check whether this is the last byte by
    // looking at the highest bit.
    if (!(b & 0x80)) {
      len_end = true;
    }
  }
  // Load the characters of the string.
  str.resize(len);
  if (len > 0) {
    infile.read(&str[0], len);
    if (!infile.good()) {
      return false;
    }
  }
  return true;
}

bool KVStore::DumpString(const string& str) {
  // Dump the length of the string with varint encoding.
  // With varint encoding, we encode integers with one or more bytes.
  // In each output byte, the most significant bit is used to indicate
  // whether there are more bytes following it, and the least significant
  // 7 bits is from the original integer. We encode from the lowest bit
  // to the highest bit.
  //
  // Example 1
  // 20 (10100) will be encoded to 00010100:
  // 1) split into groups of 7 bits:                      0010100
  // 2) reverse order to start from the lowest group:     0010100
  // 3) set highest bits as 1 except for the last group: 00010100
  //
  // Example 2
  // 499 (111110011) will be encoded to 11110011 00000011:
  // 1) split into groups of 7 bits:                      0000011  1110011
  // 2) reverse order to start from the lowest group:     1110011  0000011
  // 3) set highest bits as 1 except for the last group: 11110011 00000011
  //
  // Reference:
  // https://developers.google.com/protocol-buffers/docs/encoding#varints
  size_t x = str.length();
  do {
    // Get the next 7 bits.
    char b = x & 0x7F;
    x >>= 7;
    // Set the most significant bit of this byte
    // as 1 if there are further bytes to encode.
    if (x > 0) {
      b |= 0x80;
    }
    // Write this byte.
    if (!log_->write(&b, sizeof b)) {
      return false;
    }
  } while (x > 0);
  // Dump all characters of the string.
  if (!str.empty()) {
    if (!log_->write(str.data(), str.length())) {
      return false;
    }
  }
  return true;
}
