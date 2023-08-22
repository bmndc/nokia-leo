/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsSVGIntegerPair.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsError.h"
#include "nsMathUtils.h"
#include "SVGContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

static nsSVGAttrTearoffTable<nsSVGIntegerPair, nsSVGIntegerPair::DOMAnimatedInteger>
  sSVGFirstAnimatedIntegerTearoffTable;
static nsSVGAttrTearoffTable<nsSVGIntegerPair, nsSVGIntegerPair::DOMAnimatedInteger>
  sSVGSecondAnimatedIntegerTearoffTable;

/* Implementation */

static nsresult
ParseIntegerOptionalInteger(const nsAString& aValue,
                            int32_t aValues[2])
{
  nsCharSeparatedTokenizerTemplate<IsSVGWhitespace>
    tokenizer(aValue, ',',
              nsCharSeparatedTokenizer::SEPARATOR_OPTIONAL);
  if (tokenizer.whitespaceBeforeFirstToken()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  uint32_t i;
  for (i = 0; i < 2 && tokenizer.hasMoreTokens(); ++i) {
    if (!SVGContentUtils::ParseInteger(tokenizer.nextToken(), aValues[i])) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
  }
  if (i == 1) {
    aValues[1] = aValues[0];
  }

  if (i == 0                    ||                // Too few values.
      tokenizer.hasMoreTokens() ||                // Too many values.
      tokenizer.whitespaceAfterCurrentToken() ||  // Trailing whitespace.
      tokenizer.separatorAfterCurrentToken()) {   // Trailing comma.
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  return NS_OK;
}

nsresult
nsSVGIntegerPair::SetBaseValueString(const nsAString &aValueAsString,
                                     nsSVGElement *aSVGElement)
{
  int32_t val[2];

  nsresult rv = ParseIntegerOptionalInteger(aValueAsString, val);

  if (NS_FAILED(rv)) {
    return rv;
  }

  mBaseVal[0] = val[0];
  mBaseVal[1] = val[1];
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[0] = mBaseVal[0];
    mAnimVal[1] = mBaseVal[1];
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }

  // We don't need to call DidChange* here - we're only called by
  // nsSVGElement::ParseAttribute under Element::SetAttr,
  // which takes care of notifying.
  return NS_OK;
}

void
nsSVGIntegerPair::GetBaseValueString(nsAString &aValueAsString) const
{
  aValueAsString.Truncate();
  aValueAsString.AppendInt(mBaseVal[0]);
  if (mBaseVal[0] != mBaseVal[1]) {
    aValueAsString.AppendLiteral(", ");
    aValueAsString.AppendInt(mBaseVal[1]);
  }
}

void
nsSVGIntegerPair::SetBaseValue(int32_t aValue, PairIndex aPairIndex,
                               nsSVGElement *aSVGElement)
{
  uint32_t index = (aPairIndex == eFirst ? 0 : 1);
  if (mIsBaseSet && mBaseVal[index] == aValue) {
    return;
  }

  nsAttrValue emptyOrOldValue = aSVGElement->WillChangeIntegerPair(mAttrEnum);
  mBaseVal[index] = aValue;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[index] = aValue;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeIntegerPair(mAttrEnum, emptyOrOldValue);
}

void
nsSVGIntegerPair::SetBaseValues(int32_t aValue1, int32_t aValue2,
                                nsSVGElement *aSVGElement)
{
  if (mIsBaseSet && mBaseVal[0] == aValue1 && mBaseVal[1] == aValue2) {
    return;
  }

  nsAttrValue emptyOrOldValue = aSVGElement->WillChangeIntegerPair(mAttrEnum);
  mBaseVal[0] = aValue1;
  mBaseVal[1] = aValue2;
  mIsBaseSet = true;
  if (!mIsAnimated) {
    mAnimVal[0] = aValue1;
    mAnimVal[1] = aValue2;
  }
  else {
    aSVGElement->AnimationNeedsResample();
  }
  aSVGElement->DidChangeIntegerPair(mAttrEnum, emptyOrOldValue);
}

void
nsSVGIntegerPair::SetAnimValue(const int32_t aValue[2], nsSVGElement *aSVGElement)
{
  if (mIsAnimated && mAnimVal[0] == aValue[0] && mAnimVal[1] == aValue[1]) {
    return;
  }
  mAnimVal[0] = aValue[0];
  mAnimVal[1] = aValue[1];
  mIsAnimated = true;
  aSVGElement->DidAnimateIntegerPair(mAttrEnum);
}

already_AddRefed<SVGAnimatedInteger>
nsSVGIntegerPair::ToDOMAnimatedInteger(PairIndex aIndex,
                                       nsSVGElement* aSVGElement)
{
  RefPtr<DOMAnimatedInteger> domAnimatedInteger =
    aIndex == eFirst ? sSVGFirstAnimatedIntegerTearoffTable.GetTearoff(this) :
                       sSVGSecondAnimatedIntegerTearoffTable.GetTearoff(this);
  if (!domAnimatedInteger) {
    domAnimatedInteger = new DOMAnimatedInteger(this, aIndex, aSVGElement);
    if (aIndex == eFirst) {
      sSVGFirstAnimatedIntegerTearoffTable.AddTearoff(this, domAnimatedInteger);
    } else {
      sSVGSecondAnimatedIntegerTearoffTable.AddTearoff(this, domAnimatedInteger);
    }
  }

  return domAnimatedInteger.forget();
}

nsSVGIntegerPair::DOMAnimatedInteger::~DOMAnimatedInteger()
{
  if (mIndex == eFirst) {
    sSVGFirstAnimatedIntegerTearoffTable.RemoveTearoff(mVal);
  } else {
    sSVGSecondAnimatedIntegerTearoffTable.RemoveTearoff(mVal);
  }
}
