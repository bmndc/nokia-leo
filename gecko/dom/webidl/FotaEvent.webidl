[Constructor(DOMString type, optional FotaEventInit eventInitDict)]
interface FotaEvent : Event
{
  readonly attribute long result;
  readonly attribute long detail;
};

dictionary FotaEventInit : EventInit
{
  long result = 0;
  long detail = 0;
};
