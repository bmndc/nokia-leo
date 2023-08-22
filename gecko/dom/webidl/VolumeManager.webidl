/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

interface VolumeManager {
  // When calling requestUp/requestDown, gecko will fire the events,
  // "request-volume-up"/"request-volume-down" to FE.
  [Throws]
  void requestUp();
  [Throws]
  void requestDown();

  // Calling this function to fire "request-volume-show" event.
  [Throws]
  void requestShow();
};
