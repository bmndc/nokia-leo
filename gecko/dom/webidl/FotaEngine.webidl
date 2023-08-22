interface FotaEngine : EventTarget {
  long start();
  long stop();

  long check();
  long download();
  long cancelDownload();
  long pause();
  long resume();
  long install();
  void scheduleCheck();
  void scheduleDownload();
  void scheduleInstall();

  DOMString getEngineState();
  DOMString getCurrentVersion();
  DOMString getCheckResult();
  DOMString getPackageInfo();

  long setNumberConfig(long configId, long configValue);
  [Throws]
  long getNumberConfig(long configId);

  void notifyEvent(DOMString eventName, DOMString eventParam);
};
