/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NS_SVGCLASS_H__
#define __NS_SVGCLASS_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

namespace mozilla {
namespace dom {
class SVGAnimatedString;
} // namespace dom
} // namespace mozilla

class nsSVGClass
{

public:
  void Init() {
    mAnimVal = nullptr;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;

  void SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;
  bool IsAnimated() const
    { return !!mAnimVal; }

  already_AddRefed<mozilla::dom::SVGAnimatedString>
  ToDOMAnimatedString(nsSVGElement* aSVGElement);

private:

  nsAutoPtr<nsString> mAnimVal;
};
#endif //__NS_SVGCLASS_H__
