/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NS_SVGBOOLEAN_H__
#define __NS_SVGBOOLEAN_H__

#include "nsCOMPtr.h"
#include "nsError.h"
#include "mozilla/Attributes.h"

class nsIAtom;
class nsSVGElement;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
class SVGAnimatedBoolean;
} // namespace dom
} // namespace mozilla

class nsSVGBoolean
{

public:
  void Init(uint8_t aAttrEnum = 0xff, bool aValue = false) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
  }

  nsresult SetBaseValueAtom(const nsIAtom* aValue, nsSVGElement *aSVGElement);
  nsIAtom* GetBaseValueAtom() const;

  void SetBaseValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetAnimValue() const
    { return mAnimVal; }

  already_AddRefed<mozilla::dom::SVGAnimatedBoolean>
    ToDOMAnimatedBoolean(nsSVGElement* aSVGElement);

private:

  bool mAnimVal;
  bool mBaseVal;
  bool mIsAnimated;
  uint8_t mAttrEnum; // element specified tracking for attribute
};
#endif //__NS_SVGBOOLEAN_H__
