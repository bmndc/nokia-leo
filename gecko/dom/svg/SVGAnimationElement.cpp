/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/SVGAnimationElement.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(SVGAnimationElement, SVGAnimationElementBase)
NS_IMPL_RELEASE_INHERITED(SVGAnimationElement, SVGAnimationElementBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimationElement)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::SVGTests)
NS_INTERFACE_MAP_END_INHERITING(SVGAnimationElementBase)

NS_IMPL_CYCLE_COLLECTION_INHERITED(SVGAnimationElement,
                                   SVGAnimationElementBase,
                                   mHrefTarget)

//----------------------------------------------------------------------
// Implementation

SVGAnimationElement::SVGAnimationElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGAnimationElementBase(aNodeInfo),
    mHrefTarget(this)
{
}

SVGAnimationElement::~SVGAnimationElement()
{
}

nsresult
SVGAnimationElement::Init()
{
  nsresult rv = SVGAnimationElementBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//----------------------------------------------------------------------

const nsAttrValue*
SVGAnimationElement::GetAnimAttr(nsIAtom* aName) const
{
  return mAttrsAndChildren.GetAttr(aName, kNameSpaceID_None);
}

bool
SVGAnimationElement::GetAnimAttr(nsIAtom* aAttName,
                                 nsAString& aResult) const
{
  return GetAttr(kNameSpaceID_None, aAttName, aResult);
}

bool
SVGAnimationElement::HasAnimAttr(nsIAtom* aAttName) const
{
  return HasAttr(kNameSpaceID_None, aAttName);
}

Element*
SVGAnimationElement::GetTargetElementContent()
{
  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    return mHrefTarget.get();
  }
  MOZ_ASSERT(!mHrefTarget.get(),
             "We shouldn't have an xlink:href target "
             "if we don't have an xlink:href attribute");

  // No "xlink:href" attribute --> I should target my parent.
  nsIContent* parent = GetFlattenedTreeParent();
  return parent && parent->IsElement() ? parent->AsElement() : nullptr;
}

bool
SVGAnimationElement::GetTargetAttributeName(int32_t *aNamespaceID,
                                            nsIAtom **aLocalName) const
{
  const nsAttrValue* nameAttr
    = mAttrsAndChildren.GetAttr(nsGkAtoms::attributeName);

  if (!nameAttr)
    return false;

  NS_ASSERTION(nameAttr->Type() == nsAttrValue::eAtom,
    "attributeName should have been parsed as an atom");

  return NS_SUCCEEDED(nsContentUtils::SplitQName(
                        this, nsDependentAtomString(nameAttr->GetAtomValue()),
                        aNamespaceID, aLocalName));
}

nsSVGElement*
SVGAnimationElement::GetTargetElement()
{
  FlushAnimations();

  // We'll just call the other GetTargetElement method, and QI to the right type
  nsIContent* target = GetTargetElementContent();

  return (target && target->IsSVGElement())
           ? static_cast<nsSVGElement*>(target) : nullptr;
}

float
SVGAnimationElement::GetStartTime(ErrorResult& rv)
{
  FlushAnimations();

  rv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
  return 0.f;
}

float
SVGAnimationElement::GetCurrentTime()
{
  // Not necessary to call FlushAnimations() for this

  return 0.0f;
}

float
SVGAnimationElement::GetSimpleDuration(ErrorResult& rv)
{
  // Not necessary to call FlushAnimations() for this

  rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  return 0.f;
}

//----------------------------------------------------------------------
// nsIContent methods

nsresult
SVGAnimationElement::BindToTree(nsIDocument* aDocument,
                                nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers)
{
  MOZ_ASSERT(!mHrefTarget.get(),
             "Shouldn't have href-target yet (or it should've been cleared)");
  nsresult rv = SVGAnimationElementBase::BindToTree(aDocument, aParent,
                                                    aBindingParent,
                                                    aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv,rv);

  // XXXdholbert is GetCtx (as a check for SVG parent) still needed here?
  if (!GetCtx()) {
    // No use proceeding. We don't have an SVG parent (yet) so we won't be able
    // to register ourselves etc. Maybe next time we'll have more luck.
    // (This sort of situation will arise a lot when trees are being constructed
    // piece by piece via script)
    return NS_OK;
  }

  // Add myself to the animation controller's master set of animation elements.
  if (aDocument) {
    const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                        kNameSpaceID_XLink);
    if (href) {
      nsAutoString hrefStr;
      href->ToString(hrefStr);

      // Pass in |aParent| instead of |this| -- first argument is only used
      // for a call to GetComposedDoc(), and |this| might not have a current
      // document yet.
      UpdateHrefTarget(aParent, hrefStr);
    }
  }

  AnimationNeedsResample();

  return NS_OK;
}

void
SVGAnimationElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  mHrefTarget.Unlink();

  AnimationNeedsResample();

  SVGAnimationElementBase::UnbindFromTree(aDeep, aNullParent);
}

bool
SVGAnimationElement::ParseAttribute(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    // Deal with target-related attributes here
    if (aAttribute == nsGkAtoms::attributeName ||
        aAttribute == nsGkAtoms::attributeType) {
      aResult.ParseAtom(aValue);
      AnimationNeedsResample();
      return true;
    }
  }

  return SVGAnimationElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                                 aValue, aResult);
}

nsresult
SVGAnimationElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  nsresult rv =
    SVGAnimationElementBase::AfterSetAttr(aNamespaceID, aName, aValue,
                                          aNotify);

  if (aNamespaceID != kNameSpaceID_XLink || aName != nsGkAtoms::href)
    return rv;

  if (!aValue) {
    mHrefTarget.Unlink();
    AnimationTargetChanged();
  } else if (IsInUncomposedDoc()) {
    MOZ_ASSERT(aValue->Type() == nsAttrValue::eString,
               "Expected href attribute to be string type");
    UpdateHrefTarget(this, aValue->GetStringValue());
  } // else: we're not yet in a document -- we'll update the target on
    // next BindToTree call.

  return rv;
}

nsresult
SVGAnimationElement::UnsetAttr(int32_t aNamespaceID,
                               nsIAtom* aAttribute, bool aNotify)
{
  nsresult rv = SVGAnimationElementBase::UnsetAttr(aNamespaceID, aAttribute,
                                                   aNotify);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

bool
SVGAnimationElement::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~(eCONTENT | eANIMATION));
}

//----------------------------------------------------------------------
// SVG utility methods

void
SVGAnimationElement::ActivateByHyperlink()
{
  FlushAnimations();
}

//----------------------------------------------------------------------
// Implementation helpers

void
SVGAnimationElement::BeginElementAt(float offset, ErrorResult& rv)
{
  // Make sure the timegraph is up-to-date
  FlushAnimations();

  AnimationNeedsResample();
  // Force synchronous sample so that events resulting from this call arrive in
  // the expected order and we get an up-to-date paint.
  FlushAnimations();
}

void
SVGAnimationElement::EndElementAt(float offset, ErrorResult& rv)
{
  // Make sure the timegraph is up-to-date
  FlushAnimations();

  AnimationNeedsResample();
  // Force synchronous sample
  FlushAnimations();
}

bool
SVGAnimationElement::IsEventAttributeName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SMIL);
}

void
SVGAnimationElement::UpdateHrefTarget(nsIContent* aNodeForContext,
                                      const nsAString& aHrefStr)
{
  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI),
                                            aHrefStr, OwnerDoc(), baseURI);
  mHrefTarget.Reset(aNodeForContext, targetURI);
  AnimationTargetChanged();
}

void
SVGAnimationElement::AnimationTargetChanged()
{
  AnimationNeedsResample();
}

} // namespace dom
} // namespace mozilla
