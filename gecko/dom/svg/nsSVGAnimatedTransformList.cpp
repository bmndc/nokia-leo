/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsSVGAnimatedTransformList.h"

#include "mozilla/dom/SVGAnimatedTransformList.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "mozilla/Move.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsSVGTransform.h"
#include "SVGContentUtils.h"
#include "nsIDOMMutationEvent.h"

namespace mozilla {

using namespace dom;

nsresult
nsSVGAnimatedTransformList::SetBaseValueString(const nsAString& aValue)
{
  SVGTransformList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return SetBaseValue(newBaseValue);
}

nsresult
nsSVGAnimatedTransformList::SetBaseValue(const SVGTransformList& aValue)
{
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // We must send this notification *before* changing mBaseVal! If the length
    // of our baseVal is being reduced, our baseVal's DOM wrapper list may have
    // to remove DOM items from itself, and any removed DOM items need to copy
    // their internal counterpart values *before* we change them.
    //
    domWrapper->InternalBaseValListWillChangeLengthTo(aValue.Length());
  }

  // (This bool will be copied to our member-var, if attr-change succeeds.)
  bool hadTransform = HasTransform();

  // We don't need to call DidChange* here - we're only called by
  // nsSVGElement::ParseAttribute under Element::SetAttr,
  // which takes care of notifying.

  nsresult rv = mBaseVal.CopyFrom(aValue);
  if (NS_FAILED(rv) && domWrapper) {
    // Attempting to increase mBaseVal's length failed - reduce domWrapper
    // back to the same length:
    domWrapper->InternalBaseValListWillChangeLengthTo(mBaseVal.Length());
  } else {
    mIsAttrSet = true;
    mHadTransformBeforeLastBaseValChange = hadTransform;
  }
  return rv;
}

void
nsSVGAnimatedTransformList::ClearBaseValue()
{
  mHadTransformBeforeLastBaseValChange = HasTransform();

  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // We must send this notification *before* changing mBaseVal! (See above.)
    domWrapper->InternalBaseValListWillChangeLengthTo(0);
  }
  mBaseVal.Clear();
  mIsAttrSet = false;
  // Caller notifies
}

nsresult
nsSVGAnimatedTransformList::SetAnimValue(const SVGTransformList& aValue,
                                         nsSVGElement *aElement)
{
  bool prevSet = HasTransform() || aElement->GetAnimateMotionTransform();
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // A new animation may totally change the number of items in the animVal
    // list, replacing what was essentially a mirror of the baseVal list, or
    // else replacing and overriding an existing animation. When this happens
    // we must try and keep our animVal's DOM wrapper in sync (see the comment
    // in SVGAnimatedTransformList::InternalBaseValListWillChangeLengthTo).
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
    domWrapper->InternalAnimValListWillChangeLengthTo(aValue.Length());
  }
  if (!mAnimVal) {
    mAnimVal = new SVGTransformList();
  }
  nsresult rv = mAnimVal->CopyFrom(aValue);
  if (NS_FAILED(rv)) {
    // OOM. We clear the animation, and, importantly, ClearAnimValue() ensures
    // that mAnimVal and its DOM wrapper (if any) will have the same length!
    ClearAnimValue(aElement);
    return rv;
  }
  int32_t modType;
  if(prevSet) {
    modType = nsIDOMMutationEvent::MODIFICATION;
  } else {
    modType = nsIDOMMutationEvent::ADDITION;
  }
  aElement->DidAnimateTransformList(modType);
  return NS_OK;
}

void
nsSVGAnimatedTransformList::ClearAnimValue(nsSVGElement *aElement)
{
  SVGAnimatedTransformList *domWrapper =
    SVGAnimatedTransformList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    // When all animation ends, animVal simply mirrors baseVal, which may have
    // a different number of items to the last active animated value. We must
    // keep the length of our animVal's DOM wrapper list in sync, and again we
    // must do that before touching mAnimVal. See comments above.
    //
    domWrapper->InternalAnimValListWillChangeLengthTo(mBaseVal.Length());
  }
  mAnimVal = nullptr;
  int32_t modType;
  if (HasTransform() || aElement->GetAnimateMotionTransform()) {
    modType = nsIDOMMutationEvent::MODIFICATION;
  } else {
    modType = nsIDOMMutationEvent::REMOVAL;
  }
  aElement->DidAnimateTransformList(modType);
}

bool
nsSVGAnimatedTransformList::IsExplicitlySet() const
{
  // Like other methods of this name, we need to know when a transform value has
  // been explicitly set.
  //
  // There are three ways an animated list can become set:
  // 1) Markup -- we set mIsAttrSet to true on any successful call to
  //    SetBaseValueString and clear it on ClearBaseValue (as called by
  //    nsSVGElement::UnsetAttr or a failed nsSVGElement::ParseAttribute)
  // 2) DOM call -- simply fetching the baseVal doesn't mean the transform value
  //    has been set. It is set if that baseVal has one or more transforms in
  //    the list.
  // 3) Animation -- which will cause the mAnimVal member to be allocated
  return mIsAttrSet || !mBaseVal.IsEmpty() || mAnimVal;
}

} // namespace mozilla
