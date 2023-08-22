
#include "TouchLocation.h"

#include "mozilla/dom/TouchEvent.h"
#include "nsDOMTokenList.h"

#include <vector>

namespace mozilla {

static bool
IsTouchLocationEnabled()
{
  static bool initialized = false;
  static bool prefEnabled = false;
  if (!initialized) {
    Preferences::AddBoolVarCache(&prefEnabled, "touch.show_location");
    initialized = true;
  }
  return prefEnabled;
}

class TouchLocationCollection
{
public:

  void Append(TouchLocation* aItem) {
    mList.push_back(aItem);
  }

  void Remove(TouchLocation* aItem) {
    auto position = std::find(mList.begin(), mList.end(), aItem);
    if (position != mList.end()) {
      mList.erase(position);
    }
  }

  void HideAll(TouchLocation* aExceptItem = nullptr) {
    for (auto& item : mList) {
      if (item == aExceptItem) {
        continue;
      }
      item->SetEnable(false);
    }
  }

  std::vector<TouchLocation*> mList;
};

static TouchLocationCollection gTouchLocationCollection;

const uint32_t TouchLocation::sSupportFingers = 10;

TouchLocation::TouchLocation(nsIPresShell* aPresShell)
  : mPresShell(aPresShell)
{
  if (mPresShell)
  {
    InjectElement(mPresShell->GetDocument());
  }

  gTouchLocationCollection.Append(this);
}

TouchLocation::~TouchLocation()
{
  gTouchLocationCollection.Remove(this);

  if (mPresShell)
  {
    RemoveElement(mPresShell->GetDocument());
  }
}

void
TouchLocation::InjectElement(nsIDocument* aDocument)
{
  // Content structure of TouchLocation
  // <div class="moz-touchlocation">  <- TouchElement()
  //   <div class="cross start">        <- StartElement(0)
  //   <div class="cross start">        <- StartElement(1)
  //   ...
  //   <div class="cross start">        <- StartElement(sSupportFingers - 1)
  //   <div class="cross">              <- CrossElement(0)
  //   <div class="cross">              <- CrossElement(1)
  //   ...
  //   <div class="cross">              <- CrossElement(sSupportFingers - 1)

  ErrorResult rv;
  nsCOMPtr<Element> element = aDocument->CreateHTMLElement(nsGkAtoms::div);
  element->ClassList()->Add(NS_LITERAL_STRING("moz-touchlocation"), rv);
  element->ClassList()->Add(NS_LITERAL_STRING("none"), rv);

  // Create for start element
  for (uint32_t i = 0; i < sSupportFingers; ++i) {
    nsCOMPtr<Element> cross = aDocument->CreateHTMLElement(nsGkAtoms::div);
    cross->ClassList()->Add(NS_LITERAL_STRING("cross"), rv);
    cross->ClassList()->Add(NS_LITERAL_STRING("start"), rv);
    cross->ClassList()->Add(NS_LITERAL_STRING("none"), rv);
    element->AppendChildTo(cross, false);
  }

  for (uint32_t i = 0; i < sSupportFingers; ++i) {
    nsCOMPtr<Element> cross = aDocument->CreateHTMLElement(nsGkAtoms::div);
    cross->ClassList()->Add(NS_LITERAL_STRING("cross"), rv);
    cross->ClassList()->Add(NS_LITERAL_STRING("none"), rv);
    element->AppendChildTo(cross, false);
  }

  mTouchElementHolder = aDocument->InsertAnonymousContent(*element, rv);
}

void
TouchLocation::RemoveElement(nsIDocument* aDocument)
{
  ErrorResult rv;
  aDocument->RemoveAnonymousContent(*mTouchElementHolder, rv);
  // It's OK rv is failed since nsCanvasFrame might not exists now.
  rv.SuppressException();
}

void
TouchLocation::HandleTouchList(EventMessage aMessage, const TouchArray& aTouchList)
{
  // We leave the latest status on the screen for purpose.
  if (!aTouchList.Length()) {
    return;
  }

  SetEnable(IsTouchLocationEnabled());

  if (!mEnabled) {
    // do nothing;
    return;
  }

  MOZ_ASSERT(aTouchList.Length() <= sSupportFingers, "We only support 10 fingers.");

  uint32_t liveMask = 0;

  for (auto& touch: aTouchList) {
    uint32_t id = touch->Identifier();

    if (id >= sSupportFingers) {
      //ASSERT(id < sSupportFingers);
      continue;
    }

    liveMask |= (1 << id);

    if (!touch->mChanged) {
      continue;
    }

    uint32_t x = touch->mRefPoint.x;
    uint32_t y = touch->mRefPoint.y;
    if (aMessage == eTouchStart) {
      SetStartElementStyle(id, x, y);
    }
    SetCrossElementStyle(id, x, y);
  }

  // hide others invsible
  for (uint32_t i = 0; i < sSupportFingers; i++) {
    if (!(liveMask & (1 << i))) {
      HideStartElement(i);
      HideCrossElement(i);
    }
  }
}

void
TouchLocation::SetEnable(bool aEnabled)
{
  if (mEnabled == aEnabled) {
    return;
  }

  ErrorResult rv;
  TouchElement()->ClassList()->Toggle(NS_LITERAL_STRING("none"),
                                      Optional<bool>(!aEnabled), rv);
  mEnabled = aEnabled;

  if (mEnabled) {
    // Hide the other ToucLocations to make sure there is
    // only one TouchLocation presetned at the same time.
    gTouchLocationCollection.HideAll(this);
  }
}

void
TouchLocation::HideStartElement(TouchLocation::FingerStartId aId)
{
  MOZ_ASSERT(aId < sSupportFingers, "We must have valid Id.");
  SetChildEnableHelper(aId, nullptr, false);
}

void
TouchLocation::HideCrossElement(TouchLocation::FingerCrossId aId)
{
  MOZ_ASSERT(aId < sSupportFingers, "We must have valid Id.");
  SetChildEnableHelper(aId, nullptr, false);
}

void
TouchLocation::SetStartElementStyle(TouchLocation::FingerStartId aId, nscoord aX, nscoord aY)
{
  MOZ_ASSERT(aId < sSupportFingers, "We must have valid Id.");
  dom::Element* start = StartElement(aId);

  nsAutoString styleStr;
  // Set the offset of the start.
  styleStr.AppendPrintf("--cross-x: %dpx; --cross-y: %dpx;", aX, aY);

  ErrorResult rv;
  start->SetAttribute(NS_LITERAL_STRING("style"), styleStr, rv);

  SetChildEnableHelper(aId, start, true);
}

void
TouchLocation::SetCrossElementStyle(TouchLocation::FingerCrossId aId, nscoord aX, nscoord aY)
{
  MOZ_ASSERT(aId < sSupportFingers, "We must have valid Id.");
  dom::Element* cross = CrossElement(aId);

  nsAutoString styleStr;
  // Set the offset of the cross.
  styleStr.AppendPrintf("--cross-x: %dpx; --cross-y: %dpx;", aX, aY);
  // Set the information string of the cross.
  styleStr.AppendPrintf("--cross-x-n: %d; --cross-y-n: %d;", aX, aY);

  ErrorResult rv;
  cross->SetAttribute(NS_LITERAL_STRING("style"), styleStr, rv);

  SetChildEnableHelper(aId, cross, true);
}

void
TouchLocation::SetChildEnableHelper(TouchLocation::ChildId aId, dom::Element* aElement, bool aEnabled)
{
  MOZ_ASSERT(aId < sSupportFingers, "We must have valid Id.");

  const uint32_t idx = aId.mId;
  const uint32_t mask = 1 << idx;
  const bool enable = !!(mChildEnabled & mask);

  if (enable == aEnabled) {
    return;
  }

  // aCross may be null, call CrossElement to retrivet elements.
  dom::Element* element = aElement ? aElement : _NthChildElement(aId);

  MOZ_ASSERT(element);

  if (element) {
    ErrorResult rv;
    if (aEnabled) {
      element->ClassList()->Remove(NS_LITERAL_STRING("none"), rv);
      mChildEnabled |= mask;
    } else {
      element->ClassList()->Add(NS_LITERAL_STRING("none"), rv);
      mChildEnabled &= ~mask;
    }
  }
}

} // namespace mozilla
