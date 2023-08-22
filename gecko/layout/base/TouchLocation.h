
#ifndef TouchLocation_h__
#define TouchLocation_h__

#include "mozilla/dom/AnonymousContent.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Touch.h"
#include "mozilla/RefPtr.h"
#include "nsTArray.h"

class nsIPresShell;

namespace mozilla {

// -----------------------------------------------------------------------------
// Upon the creation of TouchLocation, it will insert DOM Element as an
// anonymous content containing the visual of touch location image.
// The location can be controlled by SetTouchList().
//
// None of the methods in AccessibleCaret will flush layout or style. To ensure
// that SetTouchList() works correctly, the caller must make sure the layout is
// up to date.
class TouchLocation
{
public:
  explicit TouchLocation(nsIPresShell* aPresShell);
  ~TouchLocation();

  using TouchArray = nsTArray<RefPtr<mozilla::dom::Touch>>;
  void HandleTouchList(EventMessage aMessage, const TouchArray& aTouchList);

  void SetEnable(bool aEnabled);

protected:

  // Helper type for ID;
  // direction: touch identify
  //         -> (euqal to) FingerStartId / FingerCrossId
  //         -> (converted) ChildId
  template<typename>
  class FingerId {
  public:
    FingerId(uint32_t aId): mId(aId) {}
    FingerId& operator= (const uint32_t& aId) { mId = aId; return *this; }
    bool operator< (const uint32_t& aA) { return mId < aA; }
    uint32_t mId;
  };

  class Start {};
  class Cross {};
  using FingerStartId = FingerId<Start>;
  using FingerCrossId = FingerId<Cross>;

  class ChildId {
  public:
    ChildId(FingerStartId& aStartId): mId(aStartId.mId) {}
    ChildId(FingerCrossId& aCrossId): mId(aCrossId.mId + sSupportFingers) {}
    uint32_t mId;
  };

  void InjectElement(nsIDocument* aDocument);
  void RemoveElement(nsIDocument* aDocument);

  dom::Element* TouchElement() const
  {
    return mTouchElementHolder->GetContentNode();
  }

  dom::Element* StartElement(FingerStartId aId) const
  {
    return aId < sSupportFingers ? _NthChildElement(aId) : nullptr;
  }

  dom::Element* CrossElement(FingerCrossId aId) const
  {
    return aId < sSupportFingers ? _NthChildElement(aId) : nullptr;
  }

  dom::Element* _NthChildElement(ChildId aId) const
  {
    const uint32_t idx = aId.mId;
    uint32_t count;
    nsIContent* const* children
        = TouchElement()->GetChildArray(&count);
    return idx < count ? children[idx]->AsElement() : nullptr;
  }

  void HideStartElement(FingerStartId aId);
  void HideCrossElement(FingerCrossId aId);

  void SetStartElementStyle(FingerStartId aId, nscoord aX, nscoord aY);
  void SetCrossElementStyle(FingerCrossId aId, nscoord aX, nscoord aY);

  void SetChildEnableHelper(ChildId aId, dom::Element* aCross, bool aEnabled);

  nsIPresShell* const mPresShell = nullptr;
  bool mEnabled = false;

  uint32_t mChildEnabled = 0;

  RefPtr<dom::AnonymousContent> mTouchElementHolder;

  static const uint32_t sSupportFingers;
}; // class TouchLocation

} // namespace mozilla

#endif // TouchLocation_h__
