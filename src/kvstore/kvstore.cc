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

enum ChangeType : char { kPut, kRemove, kClear };

KVStore::KVStore() : map_(), mutex_(), log_() {}

KVStore::KVStore(initializer_list<pair<string, vector<string>>> args)
    : map_(), mutex_(), log_() {
  for (const auto& p : args) {
    map_[p.first] = p.second;
  }
}

KVStore::KVStore(const string& filename)
    : map_(), mutex_(), log_() {
  ifstream infile(filename, ifstream::binary);
  if (infile) {
    LOG(INFO) << "Successfully opened file " << filename << " in read mode.";
    int cur_pos = infile.tellg();
    while (LoadChange(infile)) {
      cur_pos = infile.tellg();
    }
    infile.close();
    LOG(INFO) << "EOF reached.";
    if (truncate(filename.c_str(), cur_pos) != 0) {
      LOG(FATAL) << "Failed to truncate trailing content.";
    } else {
      LOG(INFO) << " Successfully truncated trailing content.";
    }
  } else {
    LOG(INFO) << "File " << filename << " does not exists, creating it...";
  }
  log_.emplace(filename, ofstream::app | ofstream::binary);
  if (!log_->is_open()) {
    LOG(FATAL) << " Failed to open file " << filename << " in write mode.";
  }
  LOG(INFO) << " Successfully opened file " << filename << " in write mode.";
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
  if (log_.has_value()) {
    char c = ChangeType::kPut;
    if (!log_->write(&c, sizeof c) || !DumpString(key) || !DumpString(value)) {
      LOG(FATAL) << "Failed to persist operation Put("
        << key << ", " << value << ") to file.";
    }
  }
  return true;
}

bool KVStore::Remove(const string& key) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  int ret = map_.erase(key);
  if (log_.has_value()) {
    char c = ChangeType::kRemove;
    if (!log_->write(&c, sizeof c) || !DumpString(key)) {
      LOG(FATAL) << "Failed to persist operation Remove("
        << key << ") to file.";
    }
  }
  return ret;
}

void KVStore::Clear() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  map_.clear();
  if (log_.has_value()) {
    char c = ChangeType::kClear;
    if (!log_->write(&c, sizeof c)) {
      LOG(FATAL) << "Failed to persist operation Clear() to file.";
    }
  }
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
      break;
    }
  }
  return true;
}

bool KVStore::LoadString(ifstream& infile, string& str) {
  size_t len = 0;
  bool len_end = false;
  int n_shifts = 0;
  while (!len_end) {
    char b;
    infile.get(b);
    if (!infile.good()) {
      return false;
    }
    len |= ((b & 0x7F) << n_shifts);
    n_shifts += 7;
    if (!(b & 0x80)) {
      len_end = true;
    }
  }
  str.resize(len);
  infile.read(&str[0], len);
  if (!infile.good() || infile.gcount() != len) {
    return false;
  }
  return true;
}

bool KVStore::DumpString(const string& str) {
  size_t x = str.length();
  while (x > 0) {
    char b = x & 0x7F;
    x >>= 7;
    if (x > 0) {
      b |= 0x80;
    }
    if (!log_->write(&b, sizeof b)) {
      return false;
    }
  }
  if(!log_->write(str.data(), str.length())) { return false; }
  return true;
}
