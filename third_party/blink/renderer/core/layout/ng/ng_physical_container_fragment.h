// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPhysicalContainerFragment_h
#define NGPhysicalContainerFragment_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "third_party/blink/renderer/core/layout/ng/ng_link.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_fragment.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class NGContainerFragmentBuilder;
enum class NGOutlineType;

class CORE_EXPORT NGPhysicalContainerFragment : public NGPhysicalFragment {
 public:
  const Vector<NGLink>& Children() const { return children_; }

  void AddOutlineRectsForNormalChildren(Vector<LayoutRect>* outline_rects,
                                        const LayoutPoint& additional_offset,
                                        NGOutlineType outline_type) const;
  void AddOutlineRectsForDescendant(const NGLink& descendant,
                                    Vector<LayoutRect>* rects,
                                    const LayoutPoint& additional_offset,
                                    NGOutlineType outline_type) const;

 protected:
  // block_or_line_writing_mode is used for converting the child offsets.
  NGPhysicalContainerFragment(NGContainerFragmentBuilder*,
                              WritingMode block_or_line_writing_mode,
                              NGFragmentType,
                              unsigned sub_type);

  Vector<NGLink> children_;
};

DEFINE_TYPE_CASTS(NGPhysicalContainerFragment,
                  NGPhysicalFragment,
                  fragment,
                  fragment->IsContainer(),
                  fragment.IsContainer());

}  // namespace blink

#endif  // NGPhysicalContainerFragment_h
