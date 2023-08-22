[Constructor(DOMString type, optional FotaCustomEventInit eventInitDict)]
interface FotaCustomEvent : Event
{
  readonly attribute DOMString name;
  readonly attribute DOMString param;
};

dictionary FotaCustomEventInit : EventInit
{
  DOMString? name  = null;
  DOMString? param = null;
};
