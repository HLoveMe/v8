// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/handles/local-handles.h"

#include "src/api/api.h"
#include "src/handles/handles.h"

namespace v8 {
namespace internal {

LocalHandles::LocalHandles() { scope_.Initialize(); }

void LocalHandles::Iterate(RootVisitor* visitor) {
  for (int i = 0; i < static_cast<int>(blocks_.size()) - 1; i++) {
    Address* block = blocks_[i];
    visitor->VisitRootPointers(Root::kHandleScope, nullptr,
                               FullObjectSlot(block),
                               FullObjectSlot(&block[kHandleBlockSize]));
  }

  if (!blocks_.empty()) {
    Address* block = blocks_.back();
    visitor->VisitRootPointers(Root::kHandleScope, nullptr,
                               FullObjectSlot(block),
                               FullObjectSlot(scope_.next));
  }
}

#ifdef DEBUG
bool LocalHandles::Contains(Address* location) {
  // We have to search in all blocks since they have no guarantee of order.
  for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
    Address* lower_bound = *it;
    // The last block is a special case because it may have less than
    // block_size_ handles.
    Address* upper_bound = lower_bound != blocks_.back()
                               ? lower_bound + kHandleBlockSize
                               : scope_.next;
    if (lower_bound <= location && location < upper_bound) {
      return true;
    }
  }
  return false;
}
#endif

Address* LocalHandles::AddBlock() {
  DCHECK_EQ(scope_.next, scope_.limit);
  Address* block = NewArray<Address>(kHandleBlockSize);
  blocks_.push_back(block);
  scope_.next = block;
  scope_.limit = block + kHandleBlockSize;
  return block;
}

void LocalHandles::RemoveBlocks() {
  while (!blocks_.empty()) {
    Address* block_start = blocks_.back();
    Address* block_limit = block_start + kHandleBlockSize;

    if (block_limit == scope_.limit) {
      break;
    }

    blocks_.pop_back();

    // TODO(dinfuehr): Zap handles in block

    DeleteArray(block_start);
  }
}

}  // namespace internal
}  // namespace v8
