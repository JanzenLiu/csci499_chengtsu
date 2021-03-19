#include "kvstore/kvstore_persistence.h"

#include <iostream>
#include <fstream>
#include <string>

using std::fstream;
using std::get;
using std::string;

namespace kvstore {

PersistenceHelper::PersistenceHelper() : log_open_(false) {}

PersistenceHelper::PersistenceHelper(const string &filename) {
  // Try to open the file.
  fstream fs(filename, fstream::in | fstream::out | fstream::binary);
  if (!fs) {
    std::cout << "File not exists, creating it: " << filename << std::endl;
    fs.open(filename,
            fstream::in | fstream::out | fstream::trunc | fstream::binary);
  }
  if (!fs.is_open()) {
    log_open_ = false;
    std::cout << "Failed to open file " << filename << std::endl;
    return;
  }
  log_.reset(new fstream(std::move(fs)));
  log_open_ = true;
}

PersistenceHelper::PersistenceHelper(
    const std::shared_ptr<std::iostream> &io) : log_(io), log_open_(true) {}

bool PersistenceHelper::DumpPut(const string& key, const string& value) {
  if (!log_open_) { return true; }
  char c = ChangeType::kPut;
  if (!log_->write(&c, sizeof c)) { return false; };
  if (!DumpString(key)) { return false; }
  if (!DumpString(value)) { return false; }
  return true;
}

bool PersistenceHelper::DumpRemove(const string& key) {
  if (!log_open_) { return true; }
  char c = ChangeType::kRemove;
  if (!log_->write(&c, sizeof c)) { return false; };
  if (!DumpString(key)) { return false; }
  return true;
}

bool PersistenceHelper::DumpClear() {
  if (!log_open_) { return true; }
  char c = ChangeType::kClear;
  if (!log_->write(&c, sizeof c)) { return false; };
  return true;
}

bool PersistenceHelper::LoadChange(Change& change) {
  if (!log_open_) { return false; }
  auto start_pos = log_->tellg();
  char type;
  log_->get(type);
  if (!log_->good()) {
    log_->seekg(start_pos);
    return false;
  }
  change.type = static_cast<ChangeType>(type);;
  switch (type) {
    case ChangeType::kPut: {
      PutParams params;
      if (!LoadString(params.key)) { return false; }
      if (!LoadString(params.value)) { return false; }
      change.params = params;
      break;
    }
    case ChangeType::kRemove: {
      RemoveParams params;
      if (!LoadString(params.key)) { return false; }
      change.params = params;
      break;
    }
    case ChangeType::kClear: {
      break;
    }
    default: {
      break;
    }
  }
  return true;
}

bool PersistenceHelper::DumpString(const string& str) {
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

bool PersistenceHelper::LoadString(std::string& str) {
  auto start_pos = log_->tellg();
  size_t len = 0;
  bool len_end = false;
  while (!len_end) {
    char b;
    log_->get(b);
    if (!log_->good()) {
      log_->seekg(start_pos);
      log_->clear();
      return false;
    }
    len <<= 7;
    len |= (b & 0x7F);
    if (!(b & 0x80)) {
      len_end = true;
    }
  }
  str.resize(len);
  log_->read(&str[0], len);
  if (!log_->good()) {
    log_->seekg(start_pos);
    log_->clear();
    return false;
  }
  return true;
}

}  // namespace kvstore