/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://www.w3.org/TR/2012/WD-dom-20120105/
 *
 * Copyright © 2012 W3C® (MIT, ERCIM, Keio), All Rights Reserved. W3C
 * liability, trademark and document use rules apply.
 */

interface DOMTokenList {
  readonly attribute unsigned long length;
  getter DOMString? item(unsigned long index);
  [Throws]
  boolean contains(DOMString token);
  [Throws]
  void add(DOMString... tokens);
  [Throws]
  void remove(DOMString... tokens);
  [Throws]
  boolean toggle(DOMString token, optional boolean force);
  [SetterThrows]
  attribute DOMString value;
  stringifier DOMString ();
  iterable<DOMString?>;
};
