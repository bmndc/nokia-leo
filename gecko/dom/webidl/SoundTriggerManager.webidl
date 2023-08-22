/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum SoundTriggerServiceState {
  "initial",
  "error",
  "stopped",
  "started",
};

enum SoundTriggerDetectMode {
  "voiceTrigger",
  "userIdentification"
};

interface SoundTriggerModuleConfig {
  /**
   * Detection mode.
   */
  readonly attribute SoundTriggerDetectMode mode;

  /**
   * The key words that will be detected by the server.
   */
  [Cached, Pure]
  readonly attribute sequence<DOMString> text;

  /**
   * Locale
   */
  readonly attribute DOMString locale;

  /**
   * The user id to this config.
   */
  readonly attribute DOMString user;

  /**
   * The uuid for the vendor.
   */
  readonly attribute DOMString vendorUUID;

  /**
   * The uuid for the sound model.
   */
  readonly attribute DOMString modelUUID;
};

interface SoundTriggerManager : EventTarget{
  /**
   * Get all supported sound models. This function should be called when service
   * is enabled.
   */
  [Throws]
  sequence<SoundTriggerModuleConfig> getSupportList();

  /**
   * Get the active sound module. If there is no active module, it throws NS_ERROR_NOT_INITIALIZED.
   * If SoundTriggerServiceState is "started", application can get which module is set by this function.
   */
  [Throws]
  SoundTriggerModuleConfig getActiveModule();

  /**
   * Set sound model configuration.
   * @param config
   *        the config that is retrieve through getSupportList.
   */
  [Throws]
  void set(SoundTriggerModuleConfig config);

  /**
   * Start recognizing
   */
  [Throws]
  void start(sequence<DOMString> keyPhrases);

  /**
   * Stop recognizing.
   */
  void stop();

  /**
   * Remove the corresponding config
   * @param config the config to be removed.
   */
  [Throws]
  void remove(SoundTriggerModuleConfig config);

  /**
   * Get the current status of soundtrigger service.
   * If the state is "enabled", application can get the list of supported sound models by "getSupportList".
   */
  readonly attribute SoundTriggerServiceState state;


  /**
   * Extend the specific sound model by user's voices.
   * @param config the config to a specific sound model.
   * @param audioData user's voices.
   * @param user user id.
   */
  [Throws]
  void extend(SoundTriggerModuleConfig config, sequence<ArrayBuffer> audioData, DOMString user);

  /**
   * When calling start, Gecko will initialize the underlying soundtrigger services.
   * After all the steps are done, Gecko will notify application by this callback.
   * NOTE. Current soundtrigger service seems only callback enable but not disable.
   */
  attribute EventHandler onstatechange;

  /**
   * Handler of the reconized events
   * When the soundtrigger service hear the key words, it callbacks the result through this handler.
   * See SoundTriggerRecognitionEvent for the details of the event.
   */
  attribute EventHandler onrecognitionresult;
};
