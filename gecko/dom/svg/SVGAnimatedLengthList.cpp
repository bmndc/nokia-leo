/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SVGAnimatedLengthList.h"

#include "DOMSVGAnimatedLengthList.h"
#include "mozilla/Move.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"

namespace mozilla {

nsresult
SVGAnimatedLengthList::SetBaseValueString(const nsAString& aValue)
{
  SVGLengthList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // We must send this notification *before* changing mBaseVal! If the length
    // of our baseVal is being reduced, our baseVal's DOM wrapper list may have
    // to remove DOM items from itself, and any removed DOM items need to copy
    // their internal counterpart values *before* we change them.
    //
    domWrapper->InternalBaseValListWillChangeTo(newBaseValue);
  }

  // We don't need to call DidChange* here - we're only called by
  // nsSVGElement::ParseAttribute under Element::SetAttr,
  // which takes care of notifying.

  rv = mBaseVal.CopyFrom(newBaseValue);
  if (NS_FAILED(rv) && domWrapper) {
    // Attempting to increase mBaseVal's length failed - reduce domWrapper
    // back to the same length:
    domWrapper->InternalBaseValListWillChangeTo(mBaseVal);
  }
  return rv;
}

void
SVGAnimatedLengthList::ClearBaseValue(uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // We must send this notification *before* changing mBaseVal! (See above.)
    domWrapper->InternalBaseValListWillChangeTo(SVGLengthList());
  }
  mBaseVal.Clear();
  // Caller notifies
}

nsresult
SVGAnimatedLengthList::SetAnimValue(const SVGLengthList& aNewAnimValue,
                                    nsSVGElement *aElement,
                                    uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // A new animation may totally change the number of items in the animVal
    // list, replacing what was essentially a mirror of the baseVal list, or
    // else replacing and overriding an existing animation. When this happens
    // we must try and keep our animVal's DOM wrapper in sync (see the comment
    // in DOMSVGAnimatedLengthList::InternalBaseValListWillChangeTo).
    //
    // It's not possible for us to reliably distinguish between calls to this
    // method that are setting a new sample for an existing animation, and
    // calls that are setting the first sample of an animation that will
    // override an existing animation. Happily it's cheap to just blindly
    // notify our animVal's DOM wrapper of its internal counterpart's new value
    // each time this method is called, so that's what we do.
    //
    // Note that we must send this notification *before* setting or changing
    // mAnimVal! (See the comment in SetBaseValueString above.)
    //
    domWrapper->InternalAnimValListWillChangeTo(aNewAnimValue);
  }
  if (!mAnimVal) {
    mAnimVal = new SVGLengthList();
  }
  nsresult rv = mAnimVal->CopyFrom(aNewAnimValue);
  if (NS_FAILED(rv)) {
    // OOM. We clear the animation, and, importantly, ClearAnimValue() ensures
    // that mAnimVal and its DOM wrapper (if any) will have the same length!
    ClearAnimValue(aElement, aAttrEnum);
    return rv;
  }
  aElement->DidAnimateLengthList(aAttrEnum);
  return NS_OK;
}

void
SVGAnimatedLengthList::ClearAnimValue(nsSVGElement *aElement,
                                      uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // When all animation ends, animVal simply mirrors baseVal, which may have
    // a different number of items to the last active animated value. We must
    // keep the length of our animVal's DOM wrapper list in sync, and again we
    // must do that before touching mAnimVal. See comments above.
    //
    domWrapper->InternalAnimValListWillChangeTo(mBaseVal);
  }
  mAnimVal = nullptr;
  aElement->DidAnimateLengthList(aAttrEnum);
}

} // namespace mozilla
